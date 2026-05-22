#include "comparator.hpp"

#include "flag_masks.hpp"
#include "x86tester_parser.hpp"

#include <iomanip>
#include <sstream>

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

  if (!row.expected_final_state.has_value()) {
    return comparison;
  }

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
          metadata.comparable_written_flags & kUserRFlagsMask);
      if (care_mask == 0) {
        AddMismatch(comparison,
                    FieldMismatch{key, expected, actual, 0,
                                  "flag output present but XED reports no "
                                  "comparable written user-mode flags"});
        continue;
      }
      if ((expected & care_mask) != (actual & care_mask)) {
        AddMismatch(comparison, FieldMismatch{key, expected, actual, care_mask,
                                              "flag mismatch"});
      }
    } else if (expected != actual) {
      AddMismatch(comparison, FieldMismatch{key, expected, actual, UINT64_MAX,
                                            "scalar mismatch"});
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
