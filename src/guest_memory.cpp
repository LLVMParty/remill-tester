#include "guest_memory.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace remill_tester {

void GuestMemory::Reset() {
  pages_.clear();
  ClearStatus();
}

void GuestMemory::ClearStatus() { last_fault_.reset(); }

bool GuestMemory::Map(std::uint64_t address, std::uint64_t size,
                      std::uint8_t permissions) {
  if (size == 0) {
    return true;
  }
  if (address > std::numeric_limits<std::uint64_t>::max() - (size - 1)) {
    SetFault(MemoryFaultKind::BackendError, address, size, "map",
             "mapping range overflows uint64_t");
    return false;
  }

  const auto start = PageBase(address);
  const auto end = PageBase(address + size - 1);
  for (auto page_address = start;; page_address += kPageSize) {
    auto &page = pages_[page_address];
    page.permissions = permissions;
    if (page_address == end) {
      break;
    }
  }
  return true;
}

bool GuestMemory::Write(std::uint64_t address,
                        const std::vector<std::uint8_t> &bytes) {
  return Write(address, bytes.data(), bytes.size());
}

bool GuestMemory::Write(std::uint64_t address, const std::uint8_t *bytes,
                        std::uint64_t size) {
  if (size == 0) {
    return true;
  }
  if (bytes == nullptr) {
    SetFault(MemoryFaultKind::BackendError, address, size, "write",
             "null source buffer");
    return false;
  }
  if (!CheckAccess(address, size, MemoryPermissions::Write, "write")) {
    return false;
  }

  std::uint64_t remaining = size;
  std::uint64_t current_address = address;
  const auto *current_bytes = bytes;
  while (remaining != 0) {
    auto *page = FindPage(current_address);
    const auto offset = PageOffset(current_address);
    const auto chunk = std::min<std::uint64_t>(remaining, kPageSize - offset);
    std::memcpy(page->bytes.data() + offset, current_bytes, chunk);
    remaining -= chunk;
    current_address += chunk;
    current_bytes += chunk;
  }
  return true;
}

bool GuestMemory::Read(std::uint64_t address, std::uint8_t *out,
                       std::uint64_t size) const {
  if (size == 0) {
    return true;
  }
  if (out == nullptr) {
    SetFault(MemoryFaultKind::BackendError, address, size, "read",
             "null destination buffer");
    return false;
  }
  if (!CheckAccess(address, size, MemoryPermissions::Read, "read")) {
    std::fill(out, out + size, 0);
    return false;
  }

  std::uint64_t remaining = size;
  std::uint64_t current_address = address;
  auto *current_out = out;
  while (remaining != 0) {
    const auto *page = FindPage(current_address);
    const auto offset = PageOffset(current_address);
    const auto chunk = std::min<std::uint64_t>(remaining, kPageSize - offset);
    std::memcpy(current_out, page->bytes.data() + offset, chunk);
    remaining -= chunk;
    current_address += chunk;
    current_out += chunk;
  }
  return true;
}

void GuestMemory::SetUnsupported(std::uint64_t address, std::string detail) {
  SetFault(MemoryFaultKind::Unsupported, address, 0, "unsupported",
           std::move(detail));
}

void GuestMemory::SetBackendError(std::string detail) {
  SetFault(MemoryFaultKind::BackendError, 0, 0, "backend", std::move(detail));
}

Memory *GuestMemory::opaque() { return reinterpret_cast<Memory *>(this); }

const Memory *GuestMemory::opaque() const {
  return reinterpret_cast<const Memory *>(this);
}

GuestMemory *GuestMemory::FromOpaque(Memory *memory) {
  return reinterpret_cast<GuestMemory *>(memory);
}

const GuestMemory *GuestMemory::FromOpaque(const Memory *memory) {
  return reinterpret_cast<const GuestMemory *>(memory);
}

std::uint64_t GuestMemory::PageBase(std::uint64_t address) {
  return address & ~(kPageSize - 1);
}

std::uint64_t GuestMemory::PageOffset(std::uint64_t address) {
  return address & (kPageSize - 1);
}

GuestMemory::Page *GuestMemory::FindPage(std::uint64_t address) {
  const auto it = pages_.find(PageBase(address));
  return it == pages_.end() ? nullptr : &it->second;
}

const GuestMemory::Page *GuestMemory::FindPage(std::uint64_t address) const {
  const auto it = pages_.find(PageBase(address));
  return it == pages_.end() ? nullptr : &it->second;
}

bool GuestMemory::CheckAccess(std::uint64_t address, std::uint64_t size,
                              std::uint8_t required_permission,
                              const char *operation) const {
  if (size == 0) {
    return true;
  }
  if (address > std::numeric_limits<std::uint64_t>::max() - (size - 1)) {
    SetFault(MemoryFaultKind::BackendError, address, size, operation,
             "access range overflows uint64_t");
    return false;
  }

  std::uint64_t remaining = size;
  std::uint64_t current_address = address;
  while (remaining != 0) {
    const auto *page = FindPage(current_address);
    const auto offset = PageOffset(current_address);
    const auto chunk = std::min<std::uint64_t>(remaining, kPageSize - offset);
    if (page == nullptr) {
      SetFault(MemoryFaultKind::Unmapped, current_address, chunk, operation,
               "page is not mapped");
      return false;
    }
    if ((page->permissions & required_permission) != required_permission) {
      SetFault(MemoryFaultKind::Permission, current_address, chunk, operation,
               "page permissions do not allow access");
      return false;
    }
    remaining -= chunk;
    current_address += chunk;
  }
  return true;
}

void GuestMemory::SetFault(MemoryFaultKind kind, std::uint64_t address,
                           std::uint64_t size, std::string operation,
                           std::string detail) const {
  if (!last_fault_.has_value()) {
    last_fault_ = MemoryFault{kind, address, size, std::move(operation),
                              std::move(detail)};
  }
}

} // namespace remill_tester
