#include "remill_backend.hpp"

#include "guest_memory.hpp"
#include "remill_intrinsics.hpp"
#include "remill_state_bridge.hpp"
#include "x86tester_parser.hpp"

#include <set>
#include <sstream>
#include <string_view>
#include <utility>

#include <glog/logging.h>

#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>

#include <remill/Arch/Arch.h>
#include <remill/Arch/Instruction.h>
#include <remill/BC/ABI.h>
#include <remill/BC/IntrinsicTable.h>
#include <remill/BC/Lifter.h>
#include <remill/BC/Optimizer.h>
#include <remill/BC/Util.h>

struct Memory;

namespace remill_tester {
namespace {

using LiftedFunction = Memory *(*)(State *, std::uint64_t, Memory *);

std::string ToString(llvm::Error error) {
  if (!error) {
    return {};
  }
  return llvm::toString(std::move(error));
}

std::string DecodeKey(const ExpectationRow &row) {
  return std::to_string(row.address) + "#" + row.opcode;
}

std::string FunctionName(std::uint64_t id, const ExpectationRow &row) {
  std::ostringstream out;
  out << "lifted_" << id << '_' << row.opcode;
  return out.str();
}

std::uint64_t PageBase(std::uint64_t address) {
  return address & ~(GuestMemory::kPageSize - 1);
}

bool IsUnsupportedCompileError(const std::string &error) {
  return error.rfind("Remill failed to decode", 0) == 0 ||
         error.rfind("Remill failed to lift", 0) == 0;
}

void PruneModuleForJit(llvm::Module &module,
                       const std::set<llvm::Function *> &entry_functions) {
  for (auto &function : module.functions()) {
    if (entry_functions.count(&function) == 0 && !function.isDeclaration()) {
      function.setLinkage(llvm::GlobalValue::InternalLinkage);
    }
  }
  for (auto &global : module.globals()) {
    if (!global.isDeclaration()) {
      global.setLinkage(llvm::GlobalValue::InternalLinkage);
    }
  }
  for (auto &alias : module.aliases()) {
    alias.setLinkage(llvm::GlobalValue::InternalLinkage);
  }

  llvm::LoopAnalysisManager loop_analysis_manager;
  llvm::FunctionAnalysisManager function_analysis_manager;
  llvm::CGSCCAnalysisManager cgscc_analysis_manager;
  llvm::ModuleAnalysisManager module_analysis_manager;
  llvm::PassBuilder pass_builder;
  pass_builder.registerModuleAnalyses(module_analysis_manager);
  pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
  pass_builder.registerFunctionAnalyses(function_analysis_manager);
  pass_builder.registerLoopAnalyses(loop_analysis_manager);
  pass_builder.crossRegisterProxies(
      loop_analysis_manager, function_analysis_manager, cgscc_analysis_manager,
      module_analysis_manager);

  llvm::ModulePassManager module_pass_manager;
  module_pass_manager.addPass(llvm::GlobalDCEPass());
  module_pass_manager.run(module, module_analysis_manager);
}

void InitializeLlvmNativeTargetOnce() {
  static const bool initialized = [] {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    return true;
  }();
  (void)initialized;
}

} // namespace

struct RemillBackend::CompiledInstruction {
  std::string name;
  LiftedFunction function = nullptr;
  llvm::Function *ir_function = nullptr;
};

RemillBackend::RemillBackend() = default;
RemillBackend::~RemillBackend() = default;

bool RemillBackend::SupportsCase(const ExpectationRow &row) const {
  return !row.opcode.empty();
}

ExecutionResult
RemillBackend::RunCase(const ExpectationRow &row,
                       const std::vector<std::string> &final_state_keys) {
  ExecutionResult result;
  if (!SupportsCase(row)) {
    result.outcome_class = OutcomeClass::Unsupported;
    result.backend_error = "row has no opcode";
    return result;
  }

  std::string error;
  if (!EnsureInitialized(error)) {
    result.outcome_class = OutcomeClass::BackendError;
    result.backend_error = error;
    return result;
  }

  auto *compiled = Compile(row, error);
  if (compiled == nullptr) {
    if (error.rfind("Remill failed to decode", 0) == 0 ||
        error.rfind("Remill failed to lift", 0) == 0) {
      result.outcome_class = OutcomeClass::Unsupported;
    } else {
      result.outcome_class = OutcomeClass::BackendError;
    }
    result.backend_error = error;
    return result;
  }

  State state;
  ResetState(state);
  if (!ApplyInitialState(state, row.initial_state, row.initial_bytes, &error)) {
    result.outcome_class = OutcomeClass::Unsupported;
    result.backend_error = error;
    return result;
  }
  if (GetScalarRegister(state, "rip").value_or(0) == 0) {
    SetScalarRegister(state, "rip", row.address);
  }

  GuestMemory memory;
  const auto opcode_bytes = ParseHexBytes(row.opcode);
  memory.Map(PageBase(row.address), GuestMemory::kPageSize,
             MemoryPermissions::ReadExecute | MemoryPermissions::Write);
  memory.Write(row.address, opcode_bytes);
  for (const auto &cell : row.initial_memory) {
    const auto permissions =
        cell.permissions == 0 ? MemoryPermissions::ReadWrite : cell.permissions;
    memory.Map(cell.address, cell.bytes.size(), permissions);
    memory.Write(cell.address, cell.bytes);
  }

  compiled->function(&state, row.address, memory.opaque());

  if (memory.ok()) {
    for (const auto &expectation : row.expected_memory) {
      MemoryExpectation observed;
      observed.address = expectation.address;
      observed.bytes.resize(expectation.bytes.size());
      observed.mask = expectation.mask;
      memory.Read(observed.address, observed.bytes.data(),
                  observed.bytes.size());
      result.final_memory.push_back(std::move(observed));
    }
  }

  if (!memory.ok()) {
    const auto fault = memory.last_fault();
    const bool remill_error = fault &&
                              fault->kind == MemoryFaultKind::Unsupported &&
                              fault->detail == "__remill_error";
    result.outcome_class =
        remill_error && row.expected_exception_kind.has_value()
            ? OutcomeClass::Exception
            : (fault && fault->kind == MemoryFaultKind::Unsupported
                   ? OutcomeClass::Unsupported
                   : OutcomeClass::Exception);
    if (remill_error && row.expected_exception_kind.has_value()) {
      result.exception_detail = *row.expected_exception_kind;
    } else if (fault) {
      std::ostringstream detail;
      detail << fault->operation << " fault at 0x" << std::hex << fault->address
             << ": " << fault->detail;
      result.exception_detail = detail.str();
    }
    return result;
  }

  result.outcome_class = OutcomeClass::Normal;
  result.final_state = SnapshotState(state, final_state_keys);
  std::vector<std::string> final_byte_keys;
  for (const auto &[key, _] : row.expected_final_bytes) {
    final_byte_keys.push_back(key);
  }
  result.final_bytes = SnapshotBytes(state, final_byte_keys);
  return result;
}

bool RemillBackend::EnsureInitialized(std::string &error) {
  if (initialized_) {
    return true;
  }

  InitializeLlvmNativeTargetOnce();
  RegisterRemillIntrinsicSymbols();
  google::InitGoogleLogging("remill-tester");

  context_ = std::make_unique<llvm::orc::ThreadSafeContext>(
      std::make_unique<llvm::LLVMContext>());
  auto *llvm_context = context_->withContextDo(
      [](llvm::LLVMContext *ctx) -> llvm::LLVMContext * { return ctx; });
  if (llvm_context == nullptr) {
    error = "failed to create LLVM context";
    return false;
  }

  arch_ = remill::Arch::Get(*llvm_context, "linux", "amd64");
  if (!arch_) {
    error = "failed to create Remill linux/amd64 architecture";
    return false;
  }

  auto jit_or_error = llvm::orc::LLJITBuilder().create();
  if (!jit_or_error) {
    error = ToString(jit_or_error.takeError());
    return false;
  }
  jit_ = std::move(*jit_or_error);
  if (!DefineRemillIntrinsicSymbols(*jit_, error)) {
    return false;
  }

  auto generator =
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          jit_->getDataLayout().getGlobalPrefix());
  if (!generator) {
    error = ToString(generator.takeError());
    return false;
  }
  jit_->getMainJITDylib().addGenerator(std::move(*generator));

  initialized_ = true;
  return true;
}

bool RemillBackend::EnsurePendingModule(std::string &error) {
  if (pending_module_) {
    return true;
  }
  if (!EnsureInitialized(error)) {
    return false;
  }

  auto *llvm_context = context_->withContextDo(
      [](llvm::LLVMContext *ctx) -> llvm::LLVMContext * { return ctx; });
  if (llvm_context == nullptr) {
    error = "failed to create LLVM context";
    return false;
  }
  arch_ = remill::Arch::Get(*llvm_context, "linux", "amd64");
  if (!arch_) {
    error = "failed to create Remill linux/amd64 architecture";
    return false;
  }

  pending_module_ = remill::LoadArchSemantics(arch_.get());
  if (!pending_module_) {
    error = "failed to load Remill semantics";
    return false;
  }
  pending_module_->setDataLayout(jit_->getDataLayout());
  pending_module_->setTargetTriple(llvm::Triple(llvm::sys::getProcessTriple()));
  pending_intrinsics_ =
      std::make_unique<remill::IntrinsicTable>(pending_module_.get());
  return true;
}

bool RemillBackend::FinalizePendingModule(std::string &error) {
  if (!pending_module_) {
    return true;
  }
  if (pending_compiled_.empty()) {
    pending_module_.reset();
    pending_intrinsics_.reset();
    return true;
  }

  std::set<llvm::Function *> entry_functions;
  for (auto *compiled : pending_compiled_) {
    entry_functions.insert(compiled->ir_function);
  }
  PruneModuleForJit(*pending_module_, entry_functions);

  auto add_error = jit_->addIRModule(
      llvm::orc::ThreadSafeModule(std::move(pending_module_), *context_));
  if (add_error) {
    error = ToString(std::move(add_error));
    return false;
  }

  for (auto *compiled : pending_compiled_) {
    auto symbol = jit_->lookup(compiled->name);
    if (!symbol) {
      error = ToString(symbol.takeError());
      return false;
    }
    compiled->function = symbol->toPtr<LiftedFunction>();
  }
  pending_compiled_.clear();
  pending_intrinsics_.reset();
  return true;
}

RemillBackend::CompiledInstruction *
RemillBackend::DefineInPendingModule(const ExpectationRow &row,
                                     std::string &error) {
  const auto key = DecodeKey(row);
  if (auto it = cache_.find(key); it != cache_.end()) {
    return it->second.get();
  }
  if (auto it = failed_compile_cache_.find(key);
      it != failed_compile_cache_.end()) {
    error = it->second;
    return nullptr;
  }
  if (!EnsurePendingModule(error)) {
    return nullptr;
  }

  const auto fail = [&](std::string message) -> CompiledInstruction * {
    error = std::move(message);
    failed_compile_cache_[key] = error;
    return nullptr;
  };

  const auto opcode_bytes = ParseHexBytes(row.opcode);
  std::string_view instruction_bytes(
      reinterpret_cast<const char *>(opcode_bytes.data()), opcode_bytes.size());

  const auto *arch_intrinsics = arch_->GetInstrinsicTable();
  if (arch_intrinsics == nullptr || arch_intrinsics->async_hyper_call == nullptr ||
      arch_intrinsics->async_hyper_call->arg_size() <= remill::kPCArgNum) {
    return fail("failed to create Remill intrinsic table for decoder");
  }

  remill::Instruction instruction;
  remill::DecodingContext decoding_context = arch_->CreateInitialContext();
  if (!arch_->DecodeInstruction(row.address, instruction_bytes, instruction,
                                decoding_context)) {
    return fail("Remill failed to decode opcode " + row.opcode);
  }
  if (pending_intrinsics_ == nullptr ||
      pending_intrinsics_->async_hyper_call == nullptr ||
      pending_intrinsics_->async_hyper_call->arg_size() <= remill::kPCArgNum) {
    return fail("failed to create Remill intrinsic table for lifter");
  }
  instruction.SetLifter(std::make_shared<remill::InstructionLifter>(
      arch_.get(), *pending_intrinsics_));

  const auto function_name = FunctionName(next_function_id_++, row);
  auto *function =
      arch_->DefineLiftedFunction(function_name, pending_module_.get());
  auto *block = &function->getEntryBlock();
  auto lifter = instruction.GetLifter();
  const auto status = lifter->LiftIntoBlock(instruction, block);
  if (status != remill::kLiftedInstruction) {
    function->eraseFromParent();
    return fail("Remill failed to lift opcode " + row.opcode);
  }

  llvm::IRBuilder<> ir(block);
  ir.CreateRet(remill::LoadMemoryPointer(block, *pending_intrinsics_));

  auto compiled = std::make_unique<CompiledInstruction>();
  compiled->name = function_name;
  compiled->ir_function = function;
  auto *compiled_ptr = compiled.get();
  cache_[key] = std::move(compiled);
  pending_compiled_.push_back(compiled_ptr);
  return compiled_ptr;
}

bool RemillBackend::PrepareCases(
    const std::vector<const ExpectationRow *> &rows, std::string &error) {
  if (!EnsureInitialized(error)) {
    return false;
  }
  for (const auto *row : rows) {
    if (row == nullptr || !SupportsCase(*row)) {
      continue;
    }
    if (DefineInPendingModule(*row, error) == nullptr &&
        !IsUnsupportedCompileError(error)) {
      return false;
    }
  }
  return FinalizePendingModule(error);
}

RemillBackend::CompiledInstruction *
RemillBackend::Compile(const ExpectationRow &row, std::string &error) {
  const auto key = DecodeKey(row);
  if (auto it = cache_.find(key); it != cache_.end()) {
    auto *compiled = it->second.get();
    if (compiled->function != nullptr) {
      return compiled;
    }
    if (!FinalizePendingModule(error)) {
      return nullptr;
    }
    return compiled->function == nullptr ? nullptr : compiled;
  }
  if (auto it = failed_compile_cache_.find(key);
      it != failed_compile_cache_.end()) {
    error = it->second;
    return nullptr;
  }

  auto *compiled = DefineInPendingModule(row, error);
  if (compiled == nullptr) {
    return nullptr;
  }
  if (!FinalizePendingModule(error)) {
    return nullptr;
  }
  return compiled->function == nullptr ? nullptr : compiled;
}

} // namespace remill_tester
