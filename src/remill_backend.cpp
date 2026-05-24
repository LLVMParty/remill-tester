#include "remill_backend.hpp"

#include "guest_memory.hpp"
#include "remill_intrinsics.hpp"
#include "remill_state_bridge.hpp"
#include "x86tester_parser.hpp"

#include <sstream>
#include <string_view>

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

void PruneModuleForJit(llvm::Module &module, llvm::Function *entry_function) {
  for (auto &function : module.functions()) {
    if (&function != entry_function && !function.isDeclaration()) {
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
  std::unique_ptr<llvm::orc::LLJIT> jit;
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
  if (!initialized_) {
    InitializeLlvmNativeTargetOnce();
    RegisterRemillIntrinsicSymbols();
    google::InitGoogleLogging("remill-tester");
    initialized_ = true;
  }
  (void)error;
  return true;
}

RemillBackend::CompiledInstruction *
RemillBackend::Compile(const ExpectationRow &row, std::string &error) {
  const auto key = DecodeKey(row);
  if (auto it = cache_.find(key); it != cache_.end()) {
    return it->second.get();
  }

  llvm::orc::ThreadSafeContext context(std::make_unique<llvm::LLVMContext>());
  auto *llvm_context = context.withContextDo(
      [](llvm::LLVMContext *ctx) -> llvm::LLVMContext * { return ctx; });
  if (llvm_context == nullptr) {
    error = "failed to create LLVM context";
    return nullptr;
  }

  auto arch = remill::Arch::Get(*llvm_context, "linux", "amd64");
  if (!arch) {
    error = "failed to create Remill linux/amd64 architecture";
    return nullptr;
  }

  auto jit_or_error = llvm::orc::LLJITBuilder().create();
  if (!jit_or_error) {
    error = ToString(jit_or_error.takeError());
    return nullptr;
  }
  auto jit = std::move(*jit_or_error);
  if (!DefineRemillIntrinsicSymbols(*jit, error)) {
    return nullptr;
  }

  auto generator =
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          jit->getDataLayout().getGlobalPrefix());
  if (!generator) {
    error = ToString(generator.takeError());
    return nullptr;
  }
  jit->getMainJITDylib().addGenerator(std::move(*generator));

  auto semantics = remill::LoadArchSemantics(arch.get());
  if (!semantics) {
    error = "failed to load Remill semantics";
    return nullptr;
  }
  const auto *intrinsics = arch->GetInstrinsicTable();
  if (intrinsics == nullptr) {
    error = "failed to create Remill intrinsic table";
    return nullptr;
  }

  const auto opcode_bytes = ParseHexBytes(row.opcode);
  std::string_view instruction_bytes(
      reinterpret_cast<const char *>(opcode_bytes.data()), opcode_bytes.size());

  remill::Instruction instruction;
  remill::DecodingContext decoding_context = arch->CreateInitialContext();
  if (!arch->DecodeInstruction(row.address, instruction_bytes, instruction,
                               decoding_context)) {
    error = "Remill failed to decode opcode " + row.opcode;
    return nullptr;
  }

  const auto function_name = FunctionName(next_function_id_++, row);
  auto *function = arch->DefineLiftedFunction(function_name, semantics.get());
  auto *block = &function->getEntryBlock();
  auto lifter = instruction.GetLifter();
  const auto status = lifter->LiftIntoBlock(instruction, block);
  if (status != remill::kLiftedInstruction) {
    error = "Remill failed to lift opcode " + row.opcode;
    return nullptr;
  }

  llvm::IRBuilder<> ir(block);
  ir.CreateRet(remill::LoadMemoryPointer(block, *intrinsics));
  remill::OptimizeModule(arch.get(), semantics.get(), {function});
  PruneModuleForJit(*semantics, function);
  semantics->setDataLayout(jit->getDataLayout());
  semantics->setTargetTriple(llvm::Triple(llvm::sys::getProcessTriple()));

  auto add_error = jit->addIRModule(
      llvm::orc::ThreadSafeModule(std::move(semantics), context));
  if (add_error) {
    error = ToString(std::move(add_error));
    return nullptr;
  }

  auto symbol = jit->lookup(function_name);
  if (!symbol) {
    error = ToString(symbol.takeError());
    return nullptr;
  }

  auto compiled = std::make_unique<CompiledInstruction>();
  compiled->name = function_name;
  compiled->function = symbol->toPtr<LiftedFunction>();
  compiled->jit = std::move(jit);
  auto *compiled_ptr = compiled.get();
  cache_[key] = std::move(compiled);
  return compiled_ptr;
}

} // namespace remill_tester
