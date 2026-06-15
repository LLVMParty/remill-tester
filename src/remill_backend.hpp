#pragma once

#include "expectation.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

namespace llvm {
class Module;
} // namespace llvm

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
  bool PrepareCases(const std::vector<const ExpectationRow *> &rows,
                    std::string &error);
  ExecutionResult RunCase(const ExpectationRow &row,
                          const std::vector<std::string> &final_state_keys);

private:
  struct CompiledInstruction;

  bool EnsureInitialized(std::string &error);
  bool EnsurePendingModule(std::string &error);
  bool FinalizePendingModule(std::string &error);
  CompiledInstruction *DefineInPendingModule(const ExpectationRow &row,
                                             std::string &error);
  CompiledInstruction *Compile(const ExpectationRow &row, std::string &error);

  bool initialized_ = false;
  std::uint64_t next_function_id_ = 0;
  std::unique_ptr<llvm::orc::ThreadSafeContext> context_;
  std::unique_ptr<const remill::Arch> arch_;
  std::unique_ptr<llvm::orc::LLJIT> jit_;
  std::unique_ptr<llvm::Module> pending_module_;
  std::unique_ptr<remill::IntrinsicTable> pending_intrinsics_;
  std::vector<CompiledInstruction *> pending_compiled_;
  std::map<std::string, std::unique_ptr<CompiledInstruction>> cache_;
  std::map<std::string, std::string> failed_compile_cache_;
};

} // namespace remill_tester
