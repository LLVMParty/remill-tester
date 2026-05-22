#pragma once

#include "expectation.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <remill/Arch/X86/Runtime/State.h>

namespace remill_tester {

const std::vector<std::string> &ScalarRegisterNames();

void ResetState(State &state);
bool IsScalarRegister(const std::string &name);
bool SetScalarRegister(State &state, const std::string &name,
                       std::uint64_t value);
std::optional<std::uint64_t> GetScalarRegister(const State &state,
                                               const std::string &name);

void SetFlags(State &state, std::uint64_t flags);
std::uint64_t GetFlags(const State &state);

bool ApplyInitialState(
    State &state, const std::map<std::string, std::uint64_t> &initial_state,
    std::string *error = nullptr);
std::map<std::string, std::uint64_t>
SnapshotState(const State &state, const std::vector<std::string> &keys);

} // namespace remill_tester
