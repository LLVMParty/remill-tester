#pragma once

#include "expectation.hpp"
#include "xed_metadata.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace remill_tester {

struct FieldMismatch {
  std::string field;
  std::uint64_t expected = 0;
  std::uint64_t actual = 0;
  std::uint64_t mask = UINT64_MAX;
  std::string detail;
};

struct ComparisonResult {
  bool passed = true;
  std::vector<FieldMismatch> mismatches;
};

ComparisonResult
CompareExecutionResult(const ExpectationRow &row, const XedMetadata &metadata,
                       const ExecutionResult &execution_result);

std::string FormatMismatch(const ExpectationRow &row,
                           const FieldMismatch &mismatch);

} // namespace remill_tester
