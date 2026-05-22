# Remill Tester Plan

## Goal

Build a tester that consumes the `3975WX/` x86Tester corpus, uses XED for x86-64 decode/flag metadata, lifts each instruction with Remill, JITs the lifted block, initializes Remill `State` plus mapped memory from each test row, executes the lifted instruction, and compares the final state against the x86Tester oracle.

Core comparison rule: compare only oracle-defined outputs, include all user-mode-visible flags that the instruction defines/reports, and mask flags that XED says are undefined.

Initial examples remain simple scalar instructions (`add`, `adc`, `xor`, `not`, `and`, `or`, `sub`, `sbb`, `cmp`, `test`, `inc`, `dec`, `lea`), but the architecture should include memory from the start so memory operands can be enabled without redesign.

## x86Tester source facts to rely on

Use `/Users/admin/Projects/x86Tester` as the authoritative description of the raw file format.

Important source locations:

- `/Users/admin/Projects/x86Tester/src/cli/main.cpp`
  - `serializeTestEntries` writes headers as:
    - `instr:0x{address};#{opcode_hex};{disassembly};{entry_count}`
  - Rows are written as:
    - ` in:<input-regs-and-flags>|out:<output-regs-and-flags>[|exception:<kind>]`
  - `outputFlags` is masked to Zydis-reported `modified | set_0 | set_1 | undefined`; therefore undefined hardware flag values are present in the file and must be masked by our comparator.
  - `inputFlags` is generated from flags read/tested and flags known set/cleared by the instruction. TF is explicitly cleared from inputs.
  - Registers are cleansed with `0xCC` before input bytes are copied, which explains values like `#04CCCCCCCCCCCCCC`.
- `/Users/admin/Projects/x86Tester/src/cli/utils.hpp`
  - `hexEncode` prints bytes in memory order.
- `/Users/admin/Projects/x86Tester/src/execution/execution.cpp`
  - `getContextReg` returns a byte span over the Windows `CONTEXT` fields.
  - Therefore register and flag blobs are little-endian host-memory bytes, not big-endian integer strings.
- `/Users/admin/Projects/x86Tester/src/generator/generator.cpp`
  - Encoded opcode bytes are emitted in normal instruction byte order.
  - Existing generator support for memory-looking operands is mainly address-generation (`AGEN`, e.g. `lea`). Current serialized rows do not contain explicit memory cells.

Format consequences:

- `#30C0` in a header is opcode bytes `30 c0`.
- `rax:#04CCCCCCCCCCCCCC` normalizes to integer `0xCCCCCCCCCCCCCC04`.
- `flags:#44000000` normalizes to integer `0x00000044`.
- Header address is the native test code address. Use it as the Remill decode/lift PC, especially for RIP-relative addressing and `lea`.
- Expected outputs are sparse. If `out:` only contains `flags`, do not compare registers unless we explicitly add an unchanged-register policy later.
- Current raw files do not serialize memory input/output. For true memory-instruction testing we must either extend x86Tester to emit memory cells or use a normalized derived schema that adds them.

## Data model

### `ExpectationRow`

```cpp
struct ExpectationRow {
  uint64_t test_case_id;
  uint64_t instruction_id;
  uint64_t address;
  std::string instruction;
  std::string opcode;
  uint64_t state_index;

  // Sparse scalar state, normalized from little-endian byte blobs.
  std::map<std::string, uint64_t> initial_state;
  std::optional<std::map<std::string, uint64_t>> expected_final_state;

  // Full-width byte values for future XMM/YMM/FPU/vector support.
  std::map<std::string, std::vector<uint8_t>> initial_bytes;
  std::map<std::string, std::vector<uint8_t>> expected_final_bytes;

  std::optional<std::string> expected_exception_kind;

  // Populated by XED, not trusted from the raw 3975WX rows.
  std::vector<std::string> undefined_flags;
  std::vector<std::string> instruction_tags;

  // Explicit mapped memory cells for the normalized/future format.
  std::vector<MemoryCell> initial_memory;
  std::vector<MemoryExpectation> expected_memory;

  uint64_t total_states = 0;
  uint64_t selected_states = 0;
  uint64_t selected_state_index = 0;
};
```

### `ExecutionResult`

```cpp
enum class OutcomeClass {
  Normal,
  Exception,
  Unsupported,
  BackendError,
};

struct ExecutionResult {
  OutcomeClass outcome_class;
  std::map<std::string, uint64_t> final_state;
  std::map<std::string, std::vector<uint8_t>> final_bytes;
  std::vector<MemoryExpectation> final_memory;
  std::optional<std::string> exception_detail;
  std::optional<std::string> backend_error;
};
```

### Memory schema

For normalized/future rows:

```cpp
struct MemoryCell {
  uint64_t address;
  std::vector<uint8_t> bytes;
  uint8_t permissions; // R/W/X bits, optional in first version.
};

struct MemoryExpectation {
  uint64_t address;
  std::vector<uint8_t> bytes;
  std::vector<uint8_t> mask; // optional byte-wise care mask.
};
```

Compatibility aliases for the conceptual Python format:

- `mem0_addr`, `mem0_value`, `mem0_size` become one `MemoryCell`.
- `expected_final_state["mem0_value"]` becomes one `MemoryExpectation`.

## Parser / normalizer

1. Read a file as a sequence of instruction groups.
2. Header grammar:
   - `instr:<address>;#<opcode-hex>;<instruction text>;<row-count>`
3. State row grammar:
   - ` in:<key:#hex,...>|out:<key:#hex,...>[|exception:<kind>]`
4. Normalize names:
   - raw `flags` -> internal `flag`
   - register names remain lowercase canonical names (`rax`, `r8`, `xmm0`, etc.)
5. Normalize values:
   - opcode hex: normal byte order
   - register/flag state hex: little-endian byte blob
   - scalar integer available for blobs of 1/2/4/8 bytes
   - always keep original bytes for future vector/FPU support
6. Derive IDs:
   - stable `instruction_id` from group index or header address/opcode tuple
   - stable `test_case_id` from monotonically increasing row id
   - `state_index` from row index within the group
7. Validate row count against header count.
8. If a row has memory-like fields from a normalized extension, populate `initial_memory` / `expected_memory`.
9. If an instruction has a real memory operand but no explicit memory oracle/input, mark it as `memory_state_missing` unless it is `lea`/address-generation-only.

## XED metadata plan

Use vendored XED from `dependencies/install` for all decode metadata.

For each unique `(address, opcode)` cache:

- decode in long 64-bit mode
- verify decoded length equals opcode byte length
- record iclass, category, extension, attributes
- record operands, including memory operand access kind and width
- record register reads/writes if practical
- record RFLAGS read/written/undefined via:
  - `xed_decoded_inst_get_rflags_info`
  - `xed_simple_flag_get_read_flag_set`
  - `xed_simple_flag_get_written_flag_set`
  - `xed_simple_flag_get_undefined_flag_set`

### Flag universe

Track the full user-mode-visible RFLAGS subset, not just arithmetic status flags:

| Bit | Name |
|---:|---|
| 0 | CF |
| 2 | PF |
| 4 | AF |
| 6 | ZF |
| 7 | SF |
| 8 | TF |
| 9 | IF |
| 10 | DF |
| 11 | OF |
| 12-13 | IOPL |
| 14 | NT |
| 16 | RF |
| 18 | AC |
| 21 | ID |

Also parse/retain XED's `VM`, `VIF`, and `VIP` bits if reported, but default comparison in long-mode user tests should not fail on reserved/virtualization-only bits unless the oracle explicitly cares and XED reports them.

Never compare reserved bits or XED's x87 pseudo flag bits (`FC0`..`FC3`) as RFLAGS.

Core masks:

```text
USER_RFLAGS_MASK = CF|PF|AF|ZF|SF|TF|IF|DF|OF|IOPL|NT|RF|AC|ID
xed_written_mask = XED written flag set & USER_RFLAGS_MASK
xed_undefined_mask = XED undefined flag set & USER_RFLAGS_MASK
comparable_written_mask = xed_written_mask & ~xed_undefined_mask
```

For raw x86Tester rows, flags in `out:` were already masked to Zydis `modified|set0|set1|undefined`. Our comparator should use XED's `comparable_written_mask`, not the raw value's populated bits, as the primary care mask.

## Remill backend plan

Implement a Remill backend with the same conceptual shape as a Unicorn backend:

```cpp
class RemillBackend {
public:
  bool supports_case(const ExpectationRow &row) const;
  ExecutionResult run_case(const ExpectationRow &row);
};
```

### Lift/JIT cache

Cache compiled lifted functions per unique opcode plus decode address where address affects semantics.

For each cache miss:

1. Create/reuse `llvm::LLVMContext` and `remill::Arch` for `linux/amd64`.
2. Load Remill amd64 semantics once.
3. Link hotpatch bitcode if needed (`helpers/x86_64/RemillHotpatch.bc`).
4. Ensure helper implementations for Remill intrinsics are available to the JIT.
5. Decode instruction bytes at `row.address`.
6. Define one lifted function with Remill ABI:
   - argument 0: `State *`
   - argument 1: `addr_t program_counter`
   - argument 2: `Memory *`
   - return: `Memory *`
7. `LiftIntoBlock` the single instruction.
8. Terminate with `ret LoadMemoryPointer(...)`.
9. Optimize after helper linking if helpers are LLVM IR and should inline.
10. Add module to ORC `LLJIT`/`ExecutionEngine`.
11. Resolve callable as:

```cpp
using LiftedFunction = Memory *(*)(State *, addr_t, Memory *);
```

### Helper/intrinsic strategy

Remill-lifted code calls many runtime intrinsics. The JIT must resolve them explicitly; do not rely on print-only IR examples.

Preferred implementation for the tester:

- Implement native `extern "C"` helper functions in the tester executable.
- Register them with the ORC JIT using absolute symbols or a current-process dynamic library search generator.
- Treat the `Memory *` argument as an opaque pointer to our `GuestMemory` object:

```cpp
auto *mem = reinterpret_cast<GuestMemory *>(memory);
```

Alternative for helpers that benefit from optimization/inlining:

- Compile/link `helpers/x86_64/RemillHelpers.bc` into the lifted module before optimization.
- Replace the current flat `extern uint8_t RAM[0]` approach with helper code that calls or casts to the `GuestMemory` mapping. A flat `RAM[a]` array is not suitable for sparse 64-bit guest addresses unless we deliberately reserve a huge virtual address space.

Minimum helper set to provide for integer/memory POC:

- memory reads/writes:
  - `__remill_read_memory_8/16/32/64`
  - `__remill_write_memory_8/16/32/64`
- floating-width memory stubs for later/vector instructions:
  - `__remill_read_memory_f32/f64/f80/f128`
  - `__remill_write_memory_f32/f64/f80/f128`
- undefined values:
  - `__remill_undefined_8/16/32/64`
  - float/vector undefined variants as needed
- flag/compare helpers:
  - `__remill_flag_computation_zero/sign/overflow/carry`
  - `__remill_compare_*`
- control-flow/hypercall/error:
  - `__remill_error`
  - `__remill_missing_block`
  - `__remill_function_call`
  - `__remill_function_return`
  - `__remill_jump`
  - `__remill_async_hyper_call`
- barriers/atomics:
  - `__remill_barrier_*`
  - `__remill_atomic_begin/end`
  - `__remill_compare_exchange_memory_*`
  - `__remill_fetch_and_{add,sub,and,or,xor,nand}_*`
- I/O and privileged hooks can return unsupported/exception for user-mode testing:
  - `__remill_read_io_port_*`
  - `__remill_write_io_port_*`
  - segment/control/debug register setters

Error handling rule: helpers should not crash the process. On unmapped memory, unsupported I/O, or `__remill_error`, set status inside `GuestMemory`/backend context and return a safe value. After the lifted call returns, convert that status into `ExecutionResult::Exception` or `BackendError`.

## Guest memory model

The memory model should mimic the useful subset of Unicorn's `mem_map`, `mem_write`, `mem_read` behavior while fitting Remill intrinsics.

### `GuestMemory`

```cpp
class GuestMemory {
public:
  bool map(uint64_t addr, uint64_t size, Permissions perms);
  bool write(uint64_t addr, std::span<const uint8_t> bytes);
  bool read(uint64_t addr, std::span<uint8_t> out) const;

  template <typename T> T read_le(uint64_t addr);
  template <typename T> void write_le(uint64_t addr, T value);

  MemoryStatus status() const;
  std::optional<MemoryFault> last_fault() const;
};
```

Implementation notes:

- Page size: `0x1000`.
- Store sparse pages in a map keyed by page-aligned guest address.
- Support reads/writes that cross page boundaries.
- Track dirty pages/ranges so expected memory checks can be sparse.
- Start with R/W permissions; add X and permission fault checks later.
- Read/write little-endian values for integer helpers.
- For atomic helpers, perform the operation on the mapped bytes and return/update by reference according to Remill's helper signatures.
- For `LOCK` operations, single-threaded execution can implement atomics non-concurrently but must preserve architectural value semantics.

### Per-row memory setup

For each row:

1. Reset `GuestMemory` to a clean map.
2. Map a code page around `row.address` if needed for PC-relative calculations/debugging. Remill does not fetch instruction bytes at runtime after lifting, but keeping the mapping makes memory diagnostics easier.
3. Write opcode bytes at `row.address` in the code page.
4. Map explicit memory cells from the row, e.g. `mem0_addr/mem0_value` or normalized `initial_memory`.
5. For stack-using instructions later, map a deterministic stack page and initialize `rsp/rbp` if absent.
6. For control-flow tests later, map a target page analogous to the Unicorn example's `CONTROL_FLOW_TARGET_PAGE_ADDR`.

If an instruction has a memory read/write operand and the row has no explicit memory cells, skip with `memory_state_missing`. Do not guess memory values from the native process.

### Memory comparison

- Compare only `expected_memory` cells supplied by the normalized row.
- Byte-wise masks allow tests to ignore undefined/untouched bytes.
- If a memory destination is written but the oracle lacks memory expectation, do not fail solely on memory; still compare listed registers/flags.

## Remill state bridge

Create a state bridge around `remill/Arch/X86/Runtime/State.h`.

Responsibilities:

- zero/default-initialize `State`
- set/get GPRs:
  - `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `rsp`, `rbp`, `r8`..`r15`
- later set/get vector/FPU state:
  - `xmm`, `ymm`, `zmm`, `st`, `mm`, `mxcsr`, x87 control/status/tag
- set/get PC/RIP if Remill state needs it for lifted semantics
- pack/unpack flags between an integer RFLAGS value and Remill's split flag representation:
  - `state.aflag` for arithmetic flags
  - `state.rflag` for architectural flags
- preserve all user-mode-visible flags listed above
- normalize required invariant bits internally if Remill expects them, especially RFLAGS bit 1, but do not compare reserved bits

Add standalone tests before instruction execution:

- parse `flags:#44000000` -> `0x44`
- pack/unpack every user-mode flag bit individually
- pack/unpack combined masks
- verify `CF` input affects `adc/sbb`
- verify `DF` survives instructions that do not write it

## Comparator plan

Comparison is sparse and mask-driven.

For each row:

1. If `expected_exception_kind` is present, require an exception outcome with matching normalized kind once exception support is enabled. Initially skip unsupported exception kinds.
2. If no exception is expected, require `OutcomeClass::Normal`.
3. For every non-flag scalar in `expected_final_state`, compare exact normalized integer value.
4. For every byte vector in `expected_final_bytes`, compare exact bytes or masked bytes.
5. For `flag`, compute:

```text
oracle_present_mask = bits present in the raw output value, only for diagnostics
care_mask = comparable_written_mask
care_mask &= USER_RFLAGS_MASK
care_mask &= ~xed_undefined_mask

actual_cmp = actual_flags & care_mask
expect_cmp = expected_flags & care_mask
```

6. If XED reports no written flags but the oracle contains `flags`, treat this as metadata mismatch and report it; do not silently compare all bits.
7. Never compare undefined flags, reserved flags, or x87 pseudo flags as RFLAGS.
8. For memory, compare only explicit `expected_memory` cells.
9. Report mismatches with:
   - file path
   - group/instruction id
   - opcode
   - instruction text
   - state index
   - field name
   - expected/actual values
   - care mask for flags/memory
   - XED undefined flags
   - skip/backend status if applicable

## Supported/skip policy

Every skipped row must have one explicit reason.

Initial supported:

- amd64/long mode
- scalar integer GPR state
- full user-mode RFLAGS mask
- immediate/register operands
- `lea` address-generation operands
- explicit mapped-memory operands where row supplies memory cells
- normal outcomes

Initial skips:

- SIMD/FPU/x87 state unless byte bridge is implemented for the involved registers
- privileged/system/I/O instructions
- expected exceptions until exception mapping is implemented
- real memory operand without explicit memory cells
- unknown Remill lift status
- unresolved JIT helper symbol
- unsupported control-flow transfer

Skip reasons:

- `xed_decode_failed`
- `xed_length_mismatch`
- `remill_decode_failed`
- `remill_lift_failed`
- `jit_compile_failed`
- `jit_symbol_missing`
- `memory_state_missing`
- `memory_fault`
- `vector_state_unsupported`
- `fpu_state_unsupported`
- `expected_exception_unsupported`
- `control_flow_unsupported`
- `privileged_or_io_unsupported`
- `backend_error`

## CLI sketch

```bash
remill-tester \
  --input 3975WX/xor.txt \
  --input 3975WX/add.txt \
  --input 3975WX/adc.txt \
  --mnemonic xor,add,adc,not,cmp,lea \
  --limit-states 1000 \
  --stop-on-first-fail
```

Useful options:

- `--input 3975WX/*.txt`
- `--opcode 4831d8`
- `--instruction "xor rax, rbx"`
- `--state-index N`
- `--limit-instructions N`
- `--limit-states N`
- `--skip-memory` / `--require-memory-oracle`
- `--dump-ir DIR`
- `--dump-lifted-module DIR`
- `--report-jsonl failures.jsonl`
- `--report-skips-jsonl skips.jsonl`
- `--stop-on-first-fail`
- `--trace-memory`
- `--trace-state`

## Proposed source layout

```text
src/
  main.cpp
  expectation.hpp/.cpp
  x86tester_parser.hpp/.cpp
  endian.hpp
  xed_metadata.hpp/.cpp
  flag_masks.hpp/.cpp
  remill_backend.hpp/.cpp
  remill_lift_cache.hpp/.cpp
  remill_jit.hpp/.cpp
  remill_state_bridge.hpp/.cpp
  remill_intrinsics.hpp/.cpp
  guest_memory.hpp/.cpp
  comparator.hpp/.cpp
  report.hpp/.cpp
```

Keep `src/example.cpp` as reference until `remill-tester` can lift/JIT and execute one instruction.

## Build plan

- Add a new executable target `remill-tester`.
- Link:
  - `LLVM-Wrapper`
  - `remill`
  - XED include/lib from `dependencies/install/include/xed` and `dependencies/install/lib/libxed.a`
- Ensure JIT can resolve native helper symbols:
  - export symbols from the executable if the platform requires it
  - or register absolute ORC symbols explicitly
- Keep helper bitcode target building; decide per milestone whether to link helper bitcode or use native callbacks.
- Add small unit/smoke tests as standalone CLI modes if no test framework is introduced yet.

## Milestones

### M0: Format and flag validation

- Document x86Tester format facts from source.
- Implement byte-order parser tests:
  - `rax:#04CCCCCCCCCCCCCC` -> `0xCCCCCCCCCCCCCC04`
  - `flags:#44000000` -> `0x44`
- Implement full user-mode RFLAGS mask table.
- Confirm XED undefined/written masks on examples:
  - `xor` marks `AF` undefined
  - `not` writes no flags
  - `adc` reads `CF` and writes status flags

### M1: Parser + XED dry run

- Parse selected files.
- Decode every unique opcode with XED.
- Print counts by mnemonic/opcode/states.
- Print written/read/undefined flags.
- Print skip reason for memory-without-oracle rows.

### M2: State bridge tests

- Pack/unpack GPRs.
- Pack/unpack all user-mode RFLAGS bits.
- Confirm Remill `State` fields map correctly for `aflag` and `rflag`.

### M3: JIT helper layer

- Bring up ORC JIT for one lifted function.
- Register native Remill intrinsic helpers.
- Implement `GuestMemory` and read/write helpers.
- Run a direct helper smoke test through JIT:
  - lifted `mov`/`xor` with no memory
  - then a small memory read/write instruction once explicit memory data is available

### M4: Single-instruction Remill smoke

- Lift/JIT one hardcoded opcode, e.g. `xor rax, rbx`.
- Initialize `State` from a handcrafted row.
- Execute `LiftedFunction(state, pc, memory)`.
- Compare `rax` and flags while masking XED-undefined `AF`.

### M5: Batch scalar runner

- Run `3975WX/xor.txt`, `not.txt`, `add.txt`, `adc.txt`, `cmp.txt`.
- Cache JIT functions by opcode/address semantics.
- Produce pass/fail/skip summary.

### M6: Memory-enabled runner

- Support explicit memory cells in normalized rows.
- Map pages and run real memory operand instructions.
- Compare expected memory cells.
- Add `--trace-memory` for read/write debugging.
- If current x86Tester data lacks memory cells, add an x86Tester extension/converter before enabling these cases.

### M7: Control flow and exceptions

- Map control-flow target pages.
- Handle Remill control-flow intrinsics.
- Map x86Tester exception strings:
  - `INT_DIVIDE_ERROR`
  - `INT_OVERFLOW`
  - `ILLEGAL_INSTRUCTION`
- Decide when Remill `__remill_error` means expected architectural exception vs backend unsupported.

### M8: Expansion

- Add SIMD/vector state.
- Add FPU/x87 and MXCSR.
- Add atomics/LOCK validation.
- Add differential optional backend against Unicorn for triage only.
- Add CI-ish regression commands for stable subsets.

## Risks / open questions

- Current 3975WX rows do not serialize memory contents; true memory instructions require extending x86Tester or normalizing to explicit memory cells.
- x86Tester uses Zydis flag metadata, while this tester will use XED. Differences in undefined/set/modified classification need diagnostics.
- Remill splits flags between `aflag` and `rflag`; incorrect packing will look like semantic failures.
- Some user-mode flags are visible but awkward in native execution (`TF`, restricted `IF/IOPL`). We should track them in state and masks, but only compare when XED/oracle says the instruction writes them.
- Undefined Remill helpers can return deterministic zero; comparator must still mask undefined flags.
- JIT helper symbol resolution is platform-sensitive.
- C++ exceptions should not cross JIT/helper boundaries; use status fields or controlled longjmp only after a deliberate design.
- Remill may not support every XED-decodable instruction or may model OS details differently from x86Tester's Windows user-mode execution.

## POC done criteria

The POC is successful when:

- parser and byte-order normalization are validated against x86Tester source behavior
- XED metadata includes all user-mode-visible flags and undefined flag masks
- Remill JIT executes selected rows from `xor`, `not`, `add`, `adc`, and `cmp`
- flag comparison masks XED-undefined bits, including `AF` for logical ops
- Remill intrinsic helpers are resolved by the JIT
- `GuestMemory` exists and is used by Remill memory helpers, even if most initial rows are register-only
- reports group pass/fail/skip by file, mnemonic, opcode, and skip reason
- at least one intentional mismatch produces a clear triage record
