#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace remill_tester {

struct MemoryCell {
  std::uint64_t address = 0;
  std::vector<std::uint8_t> bytes;
  std::uint8_t permissions = 0;
};

struct MemoryExpectation {
  std::uint64_t address = 0;
  std::vector<std::uint8_t> bytes;
  std::vector<std::uint8_t> mask;
};

struct ExpectationRow {
  std::uint64_t test_case_id = 0;
  std::uint64_t instruction_id = 0;
  std::uint64_t address = 0;
  std::string instruction;
  std::string opcode;
  std::uint64_t state_index = 0;

  std::map<std::string, std::uint64_t> initial_state;
  std::optional<std::map<std::string, std::uint64_t>> expected_final_state;

  std::map<std::string, std::vector<std::uint8_t>> initial_bytes;
  std::map<std::string, std::vector<std::uint8_t>> expected_final_bytes;

  std::optional<std::string> expected_exception_kind;

  std::vector<std::string> undefined_flags;
  std::vector<std::string> instruction_tags;

  std::vector<MemoryCell> initial_memory;
  std::vector<MemoryExpectation> expected_memory;

  std::uint64_t total_states = 0;
  std::uint64_t selected_states = 0;
  std::uint64_t selected_state_index = 0;
};

} // namespace remill_tester
