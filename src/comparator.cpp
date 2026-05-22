#include "comparator.hpp"

#include "flag_masks.hpp"
#include "remill_state_bridge.hpp"
#include "x86tester_parser.hpp"

#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>

namespace remill_tester {
namespace {

std::string OutcomeClassName(OutcomeClass outcome) {
  switch (outcome) {
  case OutcomeClass::Normal:
    return "normal";
  case OutcomeClass::Exception:
    return "exception";
  case OutcomeClass::Unsupported:
    return "unsupported";
  case OutcomeClass::BackendError:
    return "backend_error";
  }
  return "unknown";
}

void AddMismatch(ComparisonResult &result, FieldMismatch mismatch) {
  result.passed = false;
  result.mismatches.push_back(std::move(mismatch));
}

std::string BytesToHex(const std::vector<std::uint8_t> &bytes) {
  std::ostringstream out;
  out << std::hex << std::uppercase << std::setfill('0');
  for (const auto byte : bytes) {
    out << std::setw(2) << static_cast<unsigned>(byte);
  }
  return out.str();
}

std::uint64_t FirstBytesAsLittleEndian(const std::vector<std::uint8_t> &bytes) {
  std::uint64_t value = 0;
  const auto limit = std::min<std::size_t>(bytes.size(), 8);
  for (std::size_t i = 0; i < limit; ++i) {
    value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
  }
  return value;
}

bool InitialFlagsProvided(const ExpectationRow &row) {
  return row.initial_state.find("flag") != row.initial_state.end();
}

std::optional<std::uint64_t> ParseCountOperand(const ExpectationRow &row) {
  auto instruction = ToLower(row.instruction);
  const auto comma = instruction.rfind(',');
  if (comma == std::string::npos) {
    return std::nullopt;
  }
  auto count_text = Trim(instruction.substr(comma + 1));
  if (count_text == "cl") {
    if (const auto it = row.initial_state.find("rcx");
        it != row.initial_state.end()) {
      return it->second & 0xffu;
    }
    return std::nullopt;
  }
  try {
    return std::stoull(count_text, nullptr, 0);
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<std::uint64_t> MaskedCount(const ExpectationRow &row,
                                         const XedMetadata &metadata) {
  if (metadata.operand_width == 0) {
    return std::nullopt;
  }
  const auto raw_count = ParseCountOperand(row);
  if (!raw_count.has_value()) {
    return std::nullopt;
  }
  const auto count_mask = metadata.operand_width == 64 ? 0x3fu : 0x1fu;
  return *raw_count & count_mask;
}

std::optional<std::uint64_t> RotateEffectiveCount(const ExpectationRow &row,
                                                  const XedMetadata &metadata) {
  const auto masked_count = MaskedCount(row, metadata);
  if (!masked_count.has_value() || metadata.operand_width == 0) {
    return std::nullopt;
  }
  if (metadata.iclass == "ROL" || metadata.iclass == "ROR") {
    return *masked_count % metadata.operand_width;
  }
  if (metadata.iclass == "RCL" || metadata.iclass == "RCR") {
    if (metadata.operand_width < 32) {
      return *masked_count % (metadata.operand_width + 1);
    }
    return *masked_count;
  }
  return std::nullopt;
}

std::uint32_t DynamicUndefinedFlagMask(const ExpectationRow &row,
                                       const XedMetadata &metadata) {
  if (metadata.operand_width == 0) {
    return 0;
  }

  if (metadata.iclass == "SHL" || metadata.iclass == "SAL" ||
      metadata.iclass == "SHR" || metadata.iclass == "SAR") {
    const auto effective_count = MaskedCount(row, metadata);
    if (!effective_count.has_value()) {
      return 0;
    }
    if (*effective_count == 0 && !InitialFlagsProvided(row)) {
      return metadata.comparable_written_flags;
    }
    if (*effective_count >= metadata.operand_width) {
      return kFlagCF;
    }
    return 0;
  }

  if (metadata.iclass == "ROL" || metadata.iclass == "ROR" ||
      metadata.iclass == "RCL" || metadata.iclass == "RCR") {
    const auto effective_count = RotateEffectiveCount(row, metadata);
    if (effective_count.has_value() && *effective_count == 0 &&
        !InitialFlagsProvided(row)) {
      return metadata.comparable_written_flags;
    }
  }

  return 0;
}

} // namespace

ComparisonResult
CompareExecutionResult(const ExpectationRow &row, const XedMetadata &metadata,
                       const ExecutionResult &execution_result) {
  ComparisonResult comparison;

  if (row.expected_exception_kind.has_value()) {
    if (execution_result.outcome_class != OutcomeClass::Exception) {
      AddMismatch(
          comparison,
          FieldMismatch{"exception", 0, 0, 0,
                        "expected exception " + *row.expected_exception_kind +
                            ", got " +
                            OutcomeClassName(execution_result.outcome_class)});
    } else if (execution_result.exception_detail.has_value() &&
               *execution_result.exception_detail !=
                   *row.expected_exception_kind) {
      AddMismatch(comparison,
                  FieldMismatch{"exception", 0, 0, 0,
                                "expected exception " +
                                    *row.expected_exception_kind + ", got " +
                                    *execution_result.exception_detail});
    }
    return comparison;
  }

  if (execution_result.outcome_class != OutcomeClass::Normal) {
    AddMismatch(
        comparison,
        FieldMismatch{"outcome", 0, 0, 0,
                      "expected normal, got " +
                          OutcomeClassName(execution_result.outcome_class) +
                          ": " +
                          execution_result.backend_error.value_or(
                              execution_result.exception_detail.value_or(""))});
    return comparison;
  }

  if (row.expected_final_state.has_value()) {
    for (const auto &[raw_key, expected] : *row.expected_final_state) {
      const auto key = CanonicalStateKey(raw_key);
      const auto actual_it = execution_result.final_state.find(key);
      if (actual_it == execution_result.final_state.end()) {
        AddMismatch(comparison,
                    FieldMismatch{key, expected, 0, 0, "actual state missing"});
        continue;
      }

      const auto actual = actual_it->second;
      if (key == "flag") {
        const auto care_mask = static_cast<std::uint64_t>(
            metadata.comparable_written_flags & kUserRFlagsMask &
            ~DynamicUndefinedFlagMask(row, metadata));
        if (care_mask == 0) {
          continue;
        }
        if ((expected & care_mask) != (actual & care_mask)) {
          AddMismatch(comparison, FieldMismatch{key, expected, actual,
                                                care_mask, "flag mismatch"});
        }
      } else if (expected != actual) {
        AddMismatch(comparison, FieldMismatch{key, expected, actual, UINT64_MAX,
                                              "scalar mismatch"});
      }
    }
  }

  for (const auto &[raw_key, expected_bytes] : row.expected_final_bytes) {
    const auto key = CanonicalStateKey(raw_key);
    if (!IsByteRegister(key)) {
      continue;
    }
    const auto actual_it = execution_result.final_bytes.find(key);
    if (actual_it == execution_result.final_bytes.end()) {
      AddMismatch(comparison,
                  FieldMismatch{key, FirstBytesAsLittleEndian(expected_bytes),
                                0, 0, "actual byte state missing"});
      continue;
    }
    if (actual_it->second != expected_bytes) {
      std::ostringstream detail;
      detail << "byte register mismatch expected=#"
             << BytesToHex(expected_bytes) << " actual=#"
             << BytesToHex(actual_it->second);
      AddMismatch(comparison,
                  FieldMismatch{key, FirstBytesAsLittleEndian(expected_bytes),
                                FirstBytesAsLittleEndian(actual_it->second), 0,
                                detail.str()});
    }
  }

  for (std::size_t i = 0; i < row.expected_memory.size(); ++i) {
    const auto &expected_memory = row.expected_memory[i];
    if (i >= execution_result.final_memory.size()) {
      AddMismatch(comparison,
                  FieldMismatch{"mem", 0, 0, 0, "actual memory missing"});
      continue;
    }

    const auto &actual_memory = execution_result.final_memory[i];
    if (actual_memory.address != expected_memory.address ||
        actual_memory.bytes.size() != expected_memory.bytes.size()) {
      AddMismatch(comparison, FieldMismatch{"mem", expected_memory.address,
                                            actual_memory.address, 0,
                                            "memory address/size mismatch"});
      continue;
    }

    bool matches = true;
    for (std::size_t byte_index = 0; byte_index < expected_memory.bytes.size();
         ++byte_index) {
      const auto mask = byte_index < expected_memory.mask.size()
                            ? expected_memory.mask[byte_index]
                            : 0xffu;
      if ((expected_memory.bytes[byte_index] & mask) !=
          (actual_memory.bytes[byte_index] & mask)) {
        matches = false;
        break;
      }
    }
    if (!matches) {
      std::ostringstream detail;
      detail << "memory mismatch at 0x" << std::hex << std::uppercase
             << expected_memory.address << " expected=#"
             << BytesToHex(expected_memory.bytes) << " actual=#"
             << BytesToHex(actual_memory.bytes);
      AddMismatch(comparison,
                  FieldMismatch{"mem",
                                FirstBytesAsLittleEndian(expected_memory.bytes),
                                FirstBytesAsLittleEndian(actual_memory.bytes),
                                0, detail.str()});
    }
  }

  return comparison;
}

std::string FormatMismatch(const ExpectationRow &row,
                           const FieldMismatch &mismatch) {
  std::ostringstream out;
  out << "case=" << row.test_case_id << " instr=" << row.instruction_id
      << " state=" << row.state_index << " opcode=" << row.opcode << " \""
      << row.instruction << "\" field=" << mismatch.field << " expected=0x"
      << std::hex << std::uppercase << mismatch.expected << " actual=0x"
      << mismatch.actual;
  if (mismatch.mask != UINT64_MAX) {
    out << " mask=0x" << mismatch.mask << " flags="
        << FormatFlagNames(static_cast<std::uint32_t>(mismatch.mask));
  }
  out << std::dec << " detail=" << mismatch.detail;
  return out.str();
}

} // namespace remill_tester
