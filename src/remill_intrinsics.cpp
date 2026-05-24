#include "remill_intrinsics.hpp"

#include "guest_memory.hpp"

#include <cstdint>
#include <mutex>
#include <string>

#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/Orc/AbsoluteSymbols.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Error.h>

namespace {

remill_tester::GuestMemory *AsGuestMemory(Memory *memory) {
  return remill_tester::GuestMemory::FromOpaque(memory);
}

template <typename T> T ReadMemory(Memory *memory, std::uint64_t address) {
  T value = 0;
  if (auto *guest_memory = AsGuestMemory(memory)) {
    guest_memory->ReadLittleEndian(address, value);
  }
  return value;
}

template <typename T>
Memory *WriteMemory(Memory *memory, std::uint64_t address, T value) {
  if (auto *guest_memory = AsGuestMemory(memory)) {
    guest_memory->WriteLittleEndian(address, value);
  }
  return memory;
}

template <typename T, typename Operation>
Memory *FetchAndOp(Memory *memory, std::uint64_t address, T &value,
                   Operation operation) {
  if (auto *guest_memory = AsGuestMemory(memory)) {
    T current = 0;
    if (guest_memory->ReadLittleEndian(address, current)) {
      const auto input = value;
      value = current;
      guest_memory->WriteLittleEndian(
          address, static_cast<T>(operation(current, input)));
    }
  }
  return memory;
}

template <typename T>
Memory *CompareExchange(Memory *memory, std::uint64_t address, T &expected,
                        T desired) {
  if (auto *guest_memory = AsGuestMemory(memory)) {
    T current = 0;
    if (guest_memory->ReadLittleEndian(address, current)) {
      if (current == expected) {
        guest_memory->WriteLittleEndian(address, desired);
      } else {
        expected = current;
      }
    }
  }
  return memory;
}

void MarkUnsupported(Memory *memory, std::uint64_t address,
                     const char *detail) {
  if (auto *guest_memory = AsGuestMemory(memory)) {
    guest_memory->SetUnsupported(address, detail);
  }
}

} // namespace

namespace remill_tester {

void RegisterRemillIntrinsicSymbols() {
  static std::once_flag once;
  std::call_once(once, [] {
#define REMILL_TESTER_ADD_SYMBOL(name)                                         \
  llvm::sys::DynamicLibrary::AddSymbol(#name, reinterpret_cast<void *>(&name));
    REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_undefined_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_undefined_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_undefined_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_undefined_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_64)
    REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_zero)
    REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_sign)
    REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_overflow)
    REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_carry)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_sle)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_slt)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_sge)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_sgt)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_ule)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_ult)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_ugt)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_uge)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_eq)
    REMILL_TESTER_ADD_SYMBOL(__remill_compare_neq)
    REMILL_TESTER_ADD_SYMBOL(__remill_fpu_exception_clear)
    REMILL_TESTER_ADD_SYMBOL(__remill_fpu_exception_test)
    REMILL_TESTER_ADD_SYMBOL(__remill_fpu_set_rounding)
    REMILL_TESTER_ADD_SYMBOL(__remill_fpu_get_rounding)
    REMILL_TESTER_ADD_SYMBOL(__remill_barrier_load_load)
    REMILL_TESTER_ADD_SYMBOL(__remill_barrier_load_store)
    REMILL_TESTER_ADD_SYMBOL(__remill_barrier_store_load)
    REMILL_TESTER_ADD_SYMBOL(__remill_barrier_store_store)
    REMILL_TESTER_ADD_SYMBOL(__remill_atomic_begin)
    REMILL_TESTER_ADD_SYMBOL(__remill_atomic_end)
    REMILL_TESTER_ADD_SYMBOL(__remill_delay_slot_begin)
    REMILL_TESTER_ADD_SYMBOL(__remill_delay_slot_end)
    REMILL_TESTER_ADD_SYMBOL(__remill_error)
    REMILL_TESTER_ADD_SYMBOL(__remill_missing_block)
    REMILL_TESTER_ADD_SYMBOL(__remill_function_call)
    REMILL_TESTER_ADD_SYMBOL(__remill_function_return)
    REMILL_TESTER_ADD_SYMBOL(__remill_jump)
    REMILL_TESTER_ADD_SYMBOL(__remill_async_hyper_call)
    REMILL_TESTER_ADD_SYMBOL(__remill_read_io_port_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_read_io_port_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_read_io_port_32)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_io_port_8)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_io_port_16)
    REMILL_TESTER_ADD_SYMBOL(__remill_write_io_port_32)

#undef REMILL_TESTER_ADD_SYMBOL
  });
}

bool DefineRemillIntrinsicSymbols(llvm::orc::LLJIT &jit, std::string &error) {
  llvm::orc::SymbolMap symbols;
  const auto add_symbol = [&](llvm::StringRef name, auto *ptr) {
    symbols[jit.mangleAndIntern(name)] =
        llvm::orc::ExecutorSymbolDef::fromPtr(ptr);
  };

#define REMILL_TESTER_ADD_SYMBOL(name) add_symbol(#name, &name);
  REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_read_memory_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_memory_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_undefined_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_undefined_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_undefined_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_undefined_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_exchange_memory_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_add_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_sub_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_and_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_or_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_xor_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_fetch_and_nand_64)
  REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_zero)
  REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_sign)
  REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_overflow)
  REMILL_TESTER_ADD_SYMBOL(__remill_flag_computation_carry)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_sle)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_slt)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_sge)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_sgt)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_ule)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_ult)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_ugt)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_uge)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_eq)
  REMILL_TESTER_ADD_SYMBOL(__remill_compare_neq)
  REMILL_TESTER_ADD_SYMBOL(__remill_fpu_exception_clear)
  REMILL_TESTER_ADD_SYMBOL(__remill_fpu_exception_test)
  REMILL_TESTER_ADD_SYMBOL(__remill_fpu_set_rounding)
  REMILL_TESTER_ADD_SYMBOL(__remill_fpu_get_rounding)
  REMILL_TESTER_ADD_SYMBOL(__remill_barrier_load_load)
  REMILL_TESTER_ADD_SYMBOL(__remill_barrier_load_store)
  REMILL_TESTER_ADD_SYMBOL(__remill_barrier_store_load)
  REMILL_TESTER_ADD_SYMBOL(__remill_barrier_store_store)
  REMILL_TESTER_ADD_SYMBOL(__remill_atomic_begin)
  REMILL_TESTER_ADD_SYMBOL(__remill_atomic_end)
  REMILL_TESTER_ADD_SYMBOL(__remill_delay_slot_begin)
  REMILL_TESTER_ADD_SYMBOL(__remill_delay_slot_end)
  REMILL_TESTER_ADD_SYMBOL(__remill_error)
  REMILL_TESTER_ADD_SYMBOL(__remill_missing_block)
  REMILL_TESTER_ADD_SYMBOL(__remill_function_call)
  REMILL_TESTER_ADD_SYMBOL(__remill_function_return)
  REMILL_TESTER_ADD_SYMBOL(__remill_jump)
  REMILL_TESTER_ADD_SYMBOL(__remill_async_hyper_call)
  REMILL_TESTER_ADD_SYMBOL(__remill_read_io_port_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_read_io_port_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_read_io_port_32)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_io_port_8)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_io_port_16)
  REMILL_TESTER_ADD_SYMBOL(__remill_write_io_port_32)
#undef REMILL_TESTER_ADD_SYMBOL

  if (auto define_error = jit.getMainJITDylib().define(
          llvm::orc::absoluteSymbols(std::move(symbols)))) {
    error = llvm::toString(std::move(define_error));
    return false;
  }
  return true;
}

} // namespace remill_tester

extern "C" {

std::uint8_t __remill_read_memory_8(Memory *memory, std::uint64_t address) {
  return ReadMemory<std::uint8_t>(memory, address);
}

std::uint16_t __remill_read_memory_16(Memory *memory, std::uint64_t address) {
  return ReadMemory<std::uint16_t>(memory, address);
}

std::uint32_t __remill_read_memory_32(Memory *memory, std::uint64_t address) {
  return ReadMemory<std::uint32_t>(memory, address);
}

std::uint64_t __remill_read_memory_64(Memory *memory, std::uint64_t address) {
  return ReadMemory<std::uint64_t>(memory, address);
}

Memory *__remill_write_memory_8(Memory *memory, std::uint64_t address,
                                std::uint8_t value) {
  return WriteMemory(memory, address, value);
}

Memory *__remill_write_memory_16(Memory *memory, std::uint64_t address,
                                 std::uint16_t value) {
  return WriteMemory(memory, address, value);
}

Memory *__remill_write_memory_32(Memory *memory, std::uint64_t address,
                                 std::uint32_t value) {
  return WriteMemory(memory, address, value);
}

Memory *__remill_write_memory_64(Memory *memory, std::uint64_t address,
                                 std::uint64_t value) {
  return WriteMemory(memory, address, value);
}

std::uint8_t __remill_undefined_8() { return 0; }
std::uint16_t __remill_undefined_16() { return 0; }
std::uint32_t __remill_undefined_32() { return 0; }
std::uint64_t __remill_undefined_64() { return 0; }

Memory *__remill_compare_exchange_memory_8(Memory *memory,
                                           std::uint64_t address,
                                           std::uint8_t &expected,
                                           std::uint8_t desired) {
  return CompareExchange(memory, address, expected, desired);
}

Memory *__remill_compare_exchange_memory_16(Memory *memory,
                                            std::uint64_t address,
                                            std::uint16_t &expected,
                                            std::uint16_t desired) {
  return CompareExchange(memory, address, expected, desired);
}

Memory *__remill_compare_exchange_memory_32(Memory *memory,
                                            std::uint64_t address,
                                            std::uint32_t &expected,
                                            std::uint32_t desired) {
  return CompareExchange(memory, address, expected, desired);
}

Memory *__remill_compare_exchange_memory_64(Memory *memory,
                                            std::uint64_t address,
                                            std::uint64_t &expected,
                                            std::uint64_t desired) {
  return CompareExchange(memory, address, expected, desired);
}

#define DEFINE_FETCH_AND_OP(name, op)                                          \
  Memory *__remill_fetch_and_##name##_8(Memory *memory, std::uint64_t address, \
                                        std::uint8_t &value) {                 \
    return FetchAndOp<std::uint8_t>(memory, address, value, op);               \
  }                                                                            \
  Memory *__remill_fetch_and_##name##_16(                                      \
      Memory *memory, std::uint64_t address, std::uint16_t &value) {           \
    return FetchAndOp<std::uint16_t>(memory, address, value, op);              \
  }                                                                            \
  Memory *__remill_fetch_and_##name##_32(                                      \
      Memory *memory, std::uint64_t address, std::uint32_t &value) {           \
    return FetchAndOp<std::uint32_t>(memory, address, value, op);              \
  }                                                                            \
  Memory *__remill_fetch_and_##name##_64(                                      \
      Memory *memory, std::uint64_t address, std::uint64_t &value) {           \
    return FetchAndOp<std::uint64_t>(memory, address, value, op);              \
  }

DEFINE_FETCH_AND_OP(add,
                    [](auto current, auto value) { return current + value; })
DEFINE_FETCH_AND_OP(sub,
                    [](auto current, auto value) { return current - value; })
DEFINE_FETCH_AND_OP(and,
                    [](auto current, auto value) { return current & value; })
DEFINE_FETCH_AND_OP(or,
                    [](auto current, auto value) { return current | value; })
DEFINE_FETCH_AND_OP(xor, [](auto current, auto value) {
  return current ^ value;
})
DEFINE_FETCH_AND_OP(nand,
                    [](auto current, auto value) { return ~(current & value); })

#undef DEFINE_FETCH_AND_OP

bool __remill_flag_computation_zero(bool result, ...) { return result; }
bool __remill_flag_computation_sign(bool result, ...) { return result; }
bool __remill_flag_computation_overflow(bool result, ...) { return result; }
bool __remill_flag_computation_carry(bool result, ...) { return result; }

bool __remill_compare_sle(bool result) { return result; }
bool __remill_compare_slt(bool result) { return result; }
bool __remill_compare_sge(bool result) { return result; }
bool __remill_compare_sgt(bool result) { return result; }
bool __remill_compare_ule(bool result) { return result; }
bool __remill_compare_ult(bool result) { return result; }
bool __remill_compare_ugt(bool result) { return result; }
bool __remill_compare_uge(bool result) { return result; }
bool __remill_compare_eq(bool result) { return result; }
bool __remill_compare_neq(bool result) { return result; }

void __remill_fpu_exception_clear(std::int32_t) {}
std::int32_t __remill_fpu_exception_test(std::int32_t) { return 0; }
void __remill_fpu_set_rounding(std::int32_t) {}
std::int32_t __remill_fpu_get_rounding() { return 0; }

Memory *__remill_barrier_load_load(Memory *memory) { return memory; }
Memory *__remill_barrier_load_store(Memory *memory) { return memory; }
Memory *__remill_barrier_store_load(Memory *memory) { return memory; }
Memory *__remill_barrier_store_store(Memory *memory) { return memory; }
Memory *__remill_atomic_begin(Memory *memory) { return memory; }
Memory *__remill_atomic_end(Memory *memory) { return memory; }
Memory *__remill_delay_slot_begin(Memory *memory) { return memory; }
Memory *__remill_delay_slot_end(Memory *memory) { return memory; }

Memory *__remill_error(State *, std::uint64_t address, Memory *memory) {
  MarkUnsupported(memory, address, "__remill_error");
  return memory;
}

Memory *__remill_missing_block(State *, std::uint64_t address, Memory *memory) {
  MarkUnsupported(memory, address, "__remill_missing_block");
  return memory;
}

Memory *__remill_function_call(State *, std::uint64_t, Memory *memory) {
  return memory;
}

Memory *__remill_function_return(State *, std::uint64_t, Memory *memory) {
  return memory;
}

Memory *__remill_jump(State *, std::uint64_t, Memory *memory) { return memory; }

Memory *__remill_async_hyper_call(State *, std::uint64_t address,
                                  Memory *memory) {
  MarkUnsupported(memory, address, "__remill_async_hyper_call");
  return memory;
}

std::uint8_t __remill_read_io_port_8(Memory *memory, std::uint64_t address) {
  MarkUnsupported(memory, address, "__remill_read_io_port_8");
  return 0;
}

std::uint16_t __remill_read_io_port_16(Memory *memory, std::uint64_t address) {
  MarkUnsupported(memory, address, "__remill_read_io_port_16");
  return 0;
}

std::uint32_t __remill_read_io_port_32(Memory *memory, std::uint64_t address) {
  MarkUnsupported(memory, address, "__remill_read_io_port_32");
  return 0;
}

Memory *__remill_write_io_port_8(Memory *memory, std::uint64_t address,
                                 std::uint8_t) {
  MarkUnsupported(memory, address, "__remill_write_io_port_8");
  return memory;
}

Memory *__remill_write_io_port_16(Memory *memory, std::uint64_t address,
                                  std::uint16_t) {
  MarkUnsupported(memory, address, "__remill_write_io_port_16");
  return memory;
}

Memory *__remill_write_io_port_32(Memory *memory, std::uint64_t address,
                                  std::uint32_t) {
  MarkUnsupported(memory, address, "__remill_write_io_port_32");
  return memory;
}

} // extern "C"
