#include "remill_state_bridge.hpp"

#include "flag_masks.hpp"
#include "x86tester_parser.hpp"

#include <cstring>
#include <set>

namespace remill_tester {

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
    std::string *error) {
  for (const auto &[raw_key, value] : initial_state) {
    const auto key = CanonicalStateKey(raw_key);
    if (key == "flag") {
      SetFlags(state, value);
    } else if (IsScalarRegister(key)) {
      SetScalarRegister(state, key, value);
    } else if (error != nullptr) {
      *error = "unsupported scalar state key: " + key;
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

} // namespace remill_tester
