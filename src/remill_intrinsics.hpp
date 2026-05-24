#pragma once

#include <cstdint>
#include <string>

struct Memory;
struct State;

namespace llvm::orc {
class LLJIT;
}

namespace remill_tester {
void RegisterRemillIntrinsicSymbols();
bool DefineRemillIntrinsicSymbols(llvm::orc::LLJIT &jit, std::string &error);
}

extern "C" {

std::uint8_t __remill_read_memory_8(Memory *memory, std::uint64_t address);
std::uint16_t __remill_read_memory_16(Memory *memory, std::uint64_t address);
std::uint32_t __remill_read_memory_32(Memory *memory, std::uint64_t address);
std::uint64_t __remill_read_memory_64(Memory *memory, std::uint64_t address);

Memory *__remill_write_memory_8(Memory *memory, std::uint64_t address,
                                std::uint8_t value);
Memory *__remill_write_memory_16(Memory *memory, std::uint64_t address,
                                 std::uint16_t value);
Memory *__remill_write_memory_32(Memory *memory, std::uint64_t address,
                                 std::uint32_t value);
Memory *__remill_write_memory_64(Memory *memory, std::uint64_t address,
                                 std::uint64_t value);

std::uint8_t __remill_undefined_8();
std::uint16_t __remill_undefined_16();
std::uint32_t __remill_undefined_32();
std::uint64_t __remill_undefined_64();

Memory *__remill_compare_exchange_memory_8(Memory *memory,
                                           std::uint64_t address,
                                           std::uint8_t &expected,
                                           std::uint8_t desired);
Memory *__remill_compare_exchange_memory_16(Memory *memory,
                                            std::uint64_t address,
                                            std::uint16_t &expected,
                                            std::uint16_t desired);
Memory *__remill_compare_exchange_memory_32(Memory *memory,
                                            std::uint64_t address,
                                            std::uint32_t &expected,
                                            std::uint32_t desired);
Memory *__remill_compare_exchange_memory_64(Memory *memory,
                                            std::uint64_t address,
                                            std::uint64_t &expected,
                                            std::uint64_t desired);

Memory *__remill_fetch_and_add_8(Memory *memory, std::uint64_t address,
                                 std::uint8_t &value);
Memory *__remill_fetch_and_add_16(Memory *memory, std::uint64_t address,
                                  std::uint16_t &value);
Memory *__remill_fetch_and_add_32(Memory *memory, std::uint64_t address,
                                  std::uint32_t &value);
Memory *__remill_fetch_and_add_64(Memory *memory, std::uint64_t address,
                                  std::uint64_t &value);

Memory *__remill_fetch_and_sub_8(Memory *memory, std::uint64_t address,
                                 std::uint8_t &value);
Memory *__remill_fetch_and_sub_16(Memory *memory, std::uint64_t address,
                                  std::uint16_t &value);
Memory *__remill_fetch_and_sub_32(Memory *memory, std::uint64_t address,
                                  std::uint32_t &value);
Memory *__remill_fetch_and_sub_64(Memory *memory, std::uint64_t address,
                                  std::uint64_t &value);

Memory *__remill_fetch_and_and_8(Memory *memory, std::uint64_t address,
                                 std::uint8_t &value);
Memory *__remill_fetch_and_and_16(Memory *memory, std::uint64_t address,
                                  std::uint16_t &value);
Memory *__remill_fetch_and_and_32(Memory *memory, std::uint64_t address,
                                  std::uint32_t &value);
Memory *__remill_fetch_and_and_64(Memory *memory, std::uint64_t address,
                                  std::uint64_t &value);

Memory *__remill_fetch_and_or_8(Memory *memory, std::uint64_t address,
                                std::uint8_t &value);
Memory *__remill_fetch_and_or_16(Memory *memory, std::uint64_t address,
                                 std::uint16_t &value);
Memory *__remill_fetch_and_or_32(Memory *memory, std::uint64_t address,
                                 std::uint32_t &value);
Memory *__remill_fetch_and_or_64(Memory *memory, std::uint64_t address,
                                 std::uint64_t &value);

Memory *__remill_fetch_and_xor_8(Memory *memory, std::uint64_t address,
                                 std::uint8_t &value);
Memory *__remill_fetch_and_xor_16(Memory *memory, std::uint64_t address,
                                  std::uint16_t &value);
Memory *__remill_fetch_and_xor_32(Memory *memory, std::uint64_t address,
                                  std::uint32_t &value);
Memory *__remill_fetch_and_xor_64(Memory *memory, std::uint64_t address,
                                  std::uint64_t &value);

Memory *__remill_fetch_and_nand_8(Memory *memory, std::uint64_t address,
                                  std::uint8_t &value);
Memory *__remill_fetch_and_nand_16(Memory *memory, std::uint64_t address,
                                   std::uint16_t &value);
Memory *__remill_fetch_and_nand_32(Memory *memory, std::uint64_t address,
                                   std::uint32_t &value);
Memory *__remill_fetch_and_nand_64(Memory *memory, std::uint64_t address,
                                   std::uint64_t &value);

bool __remill_flag_computation_zero(bool result, ...);
bool __remill_flag_computation_sign(bool result, ...);
bool __remill_flag_computation_overflow(bool result, ...);
bool __remill_flag_computation_carry(bool result, ...);

bool __remill_compare_sle(bool result);
bool __remill_compare_slt(bool result);
bool __remill_compare_sge(bool result);
bool __remill_compare_sgt(bool result);
bool __remill_compare_ule(bool result);
bool __remill_compare_ult(bool result);
bool __remill_compare_ugt(bool result);
bool __remill_compare_uge(bool result);
bool __remill_compare_eq(bool result);
bool __remill_compare_neq(bool result);

void __remill_fpu_exception_clear(std::int32_t clear_mask);
std::int32_t __remill_fpu_exception_test(std::int32_t test_mask);
void __remill_fpu_set_rounding(std::int32_t round_mode);
std::int32_t __remill_fpu_get_rounding();

Memory *__remill_barrier_load_load(Memory *memory);
Memory *__remill_barrier_load_store(Memory *memory);
Memory *__remill_barrier_store_load(Memory *memory);
Memory *__remill_barrier_store_store(Memory *memory);
Memory *__remill_atomic_begin(Memory *memory);
Memory *__remill_atomic_end(Memory *memory);
Memory *__remill_delay_slot_begin(Memory *memory);
Memory *__remill_delay_slot_end(Memory *memory);

Memory *__remill_error(State *state, std::uint64_t address, Memory *memory);
Memory *__remill_missing_block(State *state, std::uint64_t address,
                               Memory *memory);
Memory *__remill_function_call(State *state, std::uint64_t address,
                               Memory *memory);
Memory *__remill_function_return(State *state, std::uint64_t address,
                                 Memory *memory);
Memory *__remill_jump(State *state, std::uint64_t address, Memory *memory);
Memory *__remill_async_hyper_call(State *state, std::uint64_t address,
                                  Memory *memory);

std::uint8_t __remill_read_io_port_8(Memory *memory, std::uint64_t address);
std::uint16_t __remill_read_io_port_16(Memory *memory, std::uint64_t address);
std::uint32_t __remill_read_io_port_32(Memory *memory, std::uint64_t address);
Memory *__remill_write_io_port_8(Memory *memory, std::uint64_t address,
                                 std::uint8_t value);
Memory *__remill_write_io_port_16(Memory *memory, std::uint64_t address,
                                  std::uint16_t value);
Memory *__remill_write_io_port_32(Memory *memory, std::uint64_t address,
                                  std::uint32_t value);
}
