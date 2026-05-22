#pragma once

#include "expectation.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

namespace remill {
class Arch;
class IntrinsicTable;
} // namespace remill

namespace remill_tester {

class RemillBackend {
public:
  RemillBackend();
  ~RemillBackend();

  bool SupportsCase(const ExpectationRow &row) const;
  ExecutionResult RunCase(const ExpectationRow &row,
                          const std::vector<std::string> &final_state_keys);

private:
  struct CompiledInstruction;

  bool EnsureInitialized(std::string &error);
  CompiledInstruction *Compile(const ExpectationRow &row, std::string &error);

  bool initialized_ = false;
  std::uint64_t next_function_id_ = 0;
  std::map<std::string, std::unique_ptr<CompiledInstruction>> cache_;
};

} // namespace remill_tester
