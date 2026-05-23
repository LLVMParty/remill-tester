#include "remill_state_bridge.hpp"

#include "flag_masks.hpp"
#include "x86tester_parser.hpp"

#include <charconv>
#include <cstring>
#include <set>

namespace remill_tester {
namespace {

enum class ByteRegisterKind { kVector, kMmx };

struct ByteRegisterInfo {
  ByteRegisterKind kind = ByteRegisterKind::kVector;
  std::size_t index = 0;
  std::size_t size = 0;
};

std::optional<ByteRegisterInfo>
ParseByteRegisterName(const std::string &raw_name) {
  const auto name = ToLower(raw_name);
  const struct {
    const char *prefix;
    std::size_t size;
  } prefixes[] = {{"xmm", 16}, {"ymm", 32}, {"zmm", 64}};

  for (const auto &prefix : prefixes) {
    const std::string prefix_text = prefix.prefix;
    if (name.rfind(prefix_text, 0) != 0) {
      continue;
    }
    const auto index_text = name.substr(prefix_text.size());
    if (index_text.empty()) {
      return std::nullopt;
    }
    std::size_t index = 0;
    const auto *begin = index_text.data();
    const auto *end = index_text.data() + index_text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, index, 10);
    if (ec != std::errc{} || ptr != end || index >= kNumVecRegisters) {
      return std::nullopt;
    }
    return ByteRegisterInfo{ByteRegisterKind::kVector, index, prefix.size};
  }

  if (name.rfind("mm", 0) == 0) {
    const auto index_text = name.substr(2);
    if (index_text.empty()) {
      return std::nullopt;
    }
    std::size_t index = 0;
    const auto *begin = index_text.data();
    const auto *end = index_text.data() + index_text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, index, 10);
    if (ec != std::errc{} || ptr != end || index >= 8) {
      return std::nullopt;
    }
    return ByteRegisterInfo{ByteRegisterKind::kMmx, index, 8};
  }

  return std::nullopt;
}

} // namespace

const std::vector<std::string> &ScalarRegisterNames() {
  static const std::vector<std::string> names = {
      "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp", "r8",
      "r9",  "r10", "r11", "r12", "r13", "r14", "r15", "rip"};
  return names;
}

void ResetState(State &state) {
  std::memset(&state, 0, sizeof(state));
  SetFlags(state, 0);
}

bool IsScalarRegister(const std::string &name) {
  static const std::set<std::string> names(ScalarRegisterNames().begin(),
                                           ScalarRegisterNames().end());
  return names.count(ToLower(name)) != 0;
}

bool SetScalarRegister(State &state, const std::string &raw_name,
                       std::uint64_t value) {
  const auto name = ToLower(raw_name);
#define REMILL_TESTER_SET_REG(register_name, field_name)                       \
  if (name == register_name) {                                                 \
    state.gpr.field_name.qword = value;                                        \
    return true;                                                               \
  }

  REMILL_TESTER_SET_REG("rax", rax)
  REMILL_TESTER_SET_REG("rbx", rbx)
  REMILL_TESTER_SET_REG("rcx", rcx)
  REMILL_TESTER_SET_REG("rdx", rdx)
  REMILL_TESTER_SET_REG("rsi", rsi)
  REMILL_TESTER_SET_REG("rdi", rdi)
  REMILL_TESTER_SET_REG("rsp", rsp)
  REMILL_TESTER_SET_REG("rbp", rbp)
  REMILL_TESTER_SET_REG("r8", r8)
  REMILL_TESTER_SET_REG("r9", r9)
  REMILL_TESTER_SET_REG("r10", r10)
  REMILL_TESTER_SET_REG("r11", r11)
  REMILL_TESTER_SET_REG("r12", r12)
  REMILL_TESTER_SET_REG("r13", r13)
  REMILL_TESTER_SET_REG("r14", r14)
  REMILL_TESTER_SET_REG("r15", r15)
  REMILL_TESTER_SET_REG("rip", rip)

#undef REMILL_TESTER_SET_REG
  return false;
}

std::optional<std::uint64_t> GetScalarRegister(const State &state,
                                               const std::string &raw_name) {
  const auto name = ToLower(raw_name);
#define REMILL_TESTER_GET_REG(register_name, field_name)                       \
  if (name == register_name) {                                                 \
    return state.gpr.field_name.qword;                                         \
  }

  REMILL_TESTER_GET_REG("rax", rax)
  REMILL_TESTER_GET_REG("rbx", rbx)
  REMILL_TESTER_GET_REG("rcx", rcx)
  REMILL_TESTER_GET_REG("rdx", rdx)
  REMILL_TESTER_GET_REG("rsi", rsi)
  REMILL_TESTER_GET_REG("rdi", rdi)
  REMILL_TESTER_GET_REG("rsp", rsp)
  REMILL_TESTER_GET_REG("rbp", rbp)
  REMILL_TESTER_GET_REG("r8", r8)
  REMILL_TESTER_GET_REG("r9", r9)
  REMILL_TESTER_GET_REG("r10", r10)
  REMILL_TESTER_GET_REG("r11", r11)
  REMILL_TESTER_GET_REG("r12", r12)
  REMILL_TESTER_GET_REG("r13", r13)
  REMILL_TESTER_GET_REG("r14", r14)
  REMILL_TESTER_GET_REG("r15", r15)
  REMILL_TESTER_GET_REG("rip", rip)

#undef REMILL_TESTER_GET_REG
  return std::nullopt;
}

bool IsByteRegister(const std::string &name) {
  return ParseByteRegisterName(name).has_value();
}

bool SetByteRegister(State &state, const std::string &name,
                     const std::vector<std::uint8_t> &bytes) {
  const auto info = ParseByteRegisterName(name);
  if (!info.has_value() || bytes.size() != info->size) {
    return false;
  }
  switch (info->kind) {
  case ByteRegisterKind::kVector:
    std::memcpy(&state.vec[info->index], bytes.data(), bytes.size());
    return true;
  case ByteRegisterKind::kMmx:
    std::memcpy(&state.mmx.elems[info->index].val, bytes.data(), bytes.size());
    return true;
  }
  return false;
}

std::optional<std::vector<std::uint8_t>>
GetByteRegister(const State &state, const std::string &name) {
  const auto info = ParseByteRegisterName(name);
  if (!info.has_value()) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> bytes(info->size);
  switch (info->kind) {
  case ByteRegisterKind::kVector:
    std::memcpy(bytes.data(), &state.vec[info->index], bytes.size());
    return bytes;
  case ByteRegisterKind::kMmx:
    std::memcpy(bytes.data(), &state.mmx.elems[info->index].val, bytes.size());
    return bytes;
  }
  return std::nullopt;
}

void SetFlags(State &state, std::uint64_t flags) {
  state.rflag.flat = flags | 0x2u;

  state.aflag.cf = (flags & kFlagCF) != 0;
  state.aflag.pf = (flags & kFlagPF) != 0;
  state.aflag.af = (flags & kFlagAF) != 0;
  state.aflag.zf = (flags & kFlagZF) != 0;
  state.aflag.sf = (flags & kFlagSF) != 0;
  state.aflag.df = (flags & kFlagDF) != 0;
  state.aflag.of = (flags & kFlagOF) != 0;
}

std::uint64_t GetFlags(const State &state) {
  auto flags = state.rflag.flat | 0x2u;

  const auto set_or_clear = [&flags](std::uint64_t mask, bool value) {
    if (value) {
      flags |= mask;
    } else {
      flags &= ~mask;
    }
  };

  set_or_clear(kFlagCF, state.aflag.cf != 0);
  set_or_clear(kFlagPF, state.aflag.pf != 0);
  set_or_clear(kFlagAF, state.aflag.af != 0);
  set_or_clear(kFlagZF, state.aflag.zf != 0);
  set_or_clear(kFlagSF, state.aflag.sf != 0);
  set_or_clear(kFlagDF, state.aflag.df != 0);
  set_or_clear(kFlagOF, state.aflag.of != 0);
  return flags;
}

bool ApplyInitialState(
    State &state, const std::map<std::string, std::uint64_t> &initial_state,
    const std::map<std::string, std::vector<std::uint8_t>> &initial_bytes,
    std::string *error) {
  for (const auto &[raw_key, value] : initial_state) {
    const auto key = CanonicalStateKey(raw_key);
    if (key == "flag") {
      SetFlags(state, value);
    } else if (IsScalarRegister(key)) {
      SetScalarRegister(state, key, value);
    } else if (IsByteRegister(key)) {
      continue;
    } else if (error != nullptr) {
      *error = "unsupported scalar state key: " + key;
      return false;
    } else {
      return false;
    }
  }

  for (const auto &[raw_key, bytes] : initial_bytes) {
    const auto key = CanonicalStateKey(raw_key);
    if (key == "flag" || IsScalarRegister(key)) {
      continue;
    }
    if (IsByteRegister(key)) {
      if (!SetByteRegister(state, key, bytes)) {
        if (error != nullptr) {
          *error = "invalid byte state for register: " + key;
        }
        return false;
      }
    } else if (error != nullptr) {
      *error = "unsupported byte state key: " + key;
      return false;
    } else {
      return false;
    }
  }
  return true;
}

std::map<std::string, std::uint64_t>
SnapshotState(const State &state, const std::vector<std::string> &keys) {
  std::map<std::string, std::uint64_t> snapshot;
  for (const auto &raw_key : keys) {
    const auto key = CanonicalStateKey(raw_key);
    if (key == "flag") {
      snapshot[key] = GetFlags(state);
    } else if (const auto value = GetScalarRegister(state, key)) {
      snapshot[key] = *value;
    }
  }
  return snapshot;
}

std::map<std::string, std::vector<std::uint8_t>>
SnapshotBytes(const State &state, const std::vector<std::string> &keys) {
  std::map<std::string, std::vector<std::uint8_t>> snapshot;
  for (const auto &raw_key : keys) {
    const auto key = CanonicalStateKey(raw_key);
    if (const auto bytes = GetByteRegister(state, key)) {
      snapshot[key] = *bytes;
    }
  }
  return snapshot;
}

} // namespace remill_tester
