#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

struct Memory;

namespace remill_tester {

struct MemoryPermissions {
  static constexpr std::uint8_t Read = 1u << 0;
  static constexpr std::uint8_t Write = 1u << 1;
  static constexpr std::uint8_t Execute = 1u << 2;
  static constexpr std::uint8_t ReadWrite = Read | Write;
  static constexpr std::uint8_t ReadExecute = Read | Execute;
  static constexpr std::uint8_t All = Read | Write | Execute;
};

enum class MemoryFaultKind {
  None,
  Unmapped,
  Permission,
  Unsupported,
  BackendError,
};

struct MemoryFault {
  MemoryFaultKind kind = MemoryFaultKind::None;
  std::uint64_t address = 0;
  std::uint64_t size = 0;
  std::string operation;
  std::string detail;
};

class GuestMemory {
public:
  static constexpr std::uint64_t kPageSize = 0x1000;

  GuestMemory() = default;

  void Reset();
  void ClearStatus();

  bool Map(std::uint64_t address, std::uint64_t size,
           std::uint8_t permissions = MemoryPermissions::ReadWrite);
  bool Write(std::uint64_t address, const std::vector<std::uint8_t> &bytes);
  bool Write(std::uint64_t address, const std::uint8_t *bytes,
             std::uint64_t size);
  bool Read(std::uint64_t address, std::uint8_t *out, std::uint64_t size) const;

  template <typename T>
  bool ReadLittleEndian(std::uint64_t address, T &out) const {
    static_assert(std::is_integral<T>::value, "integral type required");
    std::array<std::uint8_t, sizeof(T)> bytes{};
    if (!Read(address, bytes.data(), bytes.size())) {
      out = 0;
      return false;
    }

    typename std::make_unsigned<T>::type value = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      value |= static_cast<typename std::make_unsigned<T>::type>(bytes[i])
               << (i * 8);
    }
    out = static_cast<T>(value);
    return true;
  }

  template <typename T> bool WriteLittleEndian(std::uint64_t address, T value) {
    static_assert(std::is_integral<T>::value, "integral type required");
    using Unsigned = typename std::make_unsigned<T>::type;
    const auto unsigned_value = static_cast<Unsigned>(value);
    std::array<std::uint8_t, sizeof(T)> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      bytes[i] = static_cast<std::uint8_t>((unsigned_value >> (i * 8)) & 0xffu);
    }
    return Write(address, bytes.data(), bytes.size());
  }

  void SetUnsupported(std::uint64_t address, std::string detail);
  void SetBackendError(std::string detail);

  bool ok() const { return !last_fault_.has_value(); }
  std::optional<MemoryFault> last_fault() const { return last_fault_; }

  Memory *opaque();
  const Memory *opaque() const;
  static GuestMemory *FromOpaque(Memory *memory);
  static const GuestMemory *FromOpaque(const Memory *memory);

private:
  struct Page {
    std::array<std::uint8_t, kPageSize> bytes{};
    std::uint8_t permissions = 0;
  };

  static std::uint64_t PageBase(std::uint64_t address);
  static std::uint64_t PageOffset(std::uint64_t address);

  Page *FindPage(std::uint64_t address);
  const Page *FindPage(std::uint64_t address) const;
  bool CheckAccess(std::uint64_t address, std::uint64_t size,
                   std::uint8_t required_permission,
                   const char *operation) const;
  void SetFault(MemoryFaultKind kind, std::uint64_t address, std::uint64_t size,
                std::string operation, std::string detail) const;

  std::map<std::uint64_t, Page> pages_;
  mutable std::optional<MemoryFault> last_fault_;
};

} // namespace remill_tester
