#include "flag_masks.hpp"
#include "guest_memory.hpp"
#include "remill_backend.hpp"
#include "remill_intrinsics.hpp"
#include "remill_state_bridge.hpp"
#include "x86tester_parser.hpp"
#include "xed_metadata.hpp"

#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace remill_tester {
namespace {

struct Options {
  std::vector<std::filesystem::path> inputs;
  std::set<std::string> mnemonics;
  std::set<std::string> opcodes;
  std::uint64_t limit_states = 0;
  std::uint64_t limit_instructions = 0;
  bool print_instructions = false;
  bool self_test = false;
  bool help = false;
};

struct Summary {
  std::uint64_t files = 0;
  std::uint64_t instruction_groups = 0;
  std::uint64_t rows = 0;
  std::uint64_t selected_rows = 0;
  std::uint64_t selected_instruction_groups = 0;
  std::uint64_t parse_warnings = 0;
  std::uint64_t unique_decodes = 0;
  std::uint64_t xed_decode_failures = 0;
  std::uint64_t xed_length_mismatches = 0;
  std::uint64_t memory_rows_without_oracle = 0;
  std::map<std::string, std::uint64_t> rows_by_mnemonic;
  std::map<std::string, std::uint64_t> groups_by_mnemonic;
  std::map<std::string, std::uint64_t> undefined_flag_mentions;
};

std::vector<std::string> SplitCsv(const std::string &text) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto pos = text.find(',', start);
    auto part = Trim(text.substr(
        start, pos == std::string::npos ? std::string::npos : pos - start));
    if (!part.empty()) {
      parts.push_back(part);
    }
    if (pos == std::string::npos) {
      break;
    }
    start = pos + 1;
  }
  return parts;
}

std::string FirstToken(const std::string &text) {
  const auto trimmed = Trim(text);
  const auto pos = trimmed.find_first_of(" \t");
  return ToLower(pos == std::string::npos ? trimmed : trimmed.substr(0, pos));
}

std::string DecodeKey(const ExpectationRow &row) {
  return std::to_string(row.address) + "#" + row.opcode;
}

std::string InstructionGroupKey(const std::filesystem::path &path,
                                const ExpectationRow &row) {
  return path.string() + "#" + std::to_string(row.instruction_id);
}

void PrintUsage(const char *argv0) {
  std::cout
      << "Usage: " << argv0 << " [options] --input 3975WX/xor.txt\n\n"
      << "Options:\n"
      << "  --input <path>             Add one x86Tester file. Positional "
         "paths also work.\n"
      << "  --mnemonic <csv>           Filter by mnemonic, e.g. xor,add,adc.\n"
      << "  --opcode <csv>             Filter by opcode hex, e.g. 4831D8.\n"
      << "  --limit-states <n>         Stop after selecting n state rows.\n"
      << "  --limit-instructions <n>   Stop after selecting n instruction "
         "groups.\n"
      << "  --print-instructions       Print one XED metadata line per "
         "selected opcode/address.\n"
      << "  --self-test                Run parser/XED smoke tests.\n"
      << "  --help                     Show this help.\n";
}

Options ParseOptions(int argc, char **argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto require_value = [&](const char *name) -> std::string {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++i];
    };

    if (arg == "--help" || arg == "-h") {
      options.help = true;
    } else if (arg == "--self-test") {
      options.self_test = true;
    } else if (arg == "--input") {
      options.inputs.emplace_back(require_value("--input"));
    } else if (arg == "--mnemonic") {
      for (auto value : SplitCsv(require_value("--mnemonic"))) {
        options.mnemonics.insert(ToLower(std::move(value)));
      }
    } else if (arg == "--opcode") {
      for (auto value : SplitCsv(require_value("--opcode"))) {
        options.opcodes.insert(ToUpper(std::move(value)));
      }
    } else if (arg == "--limit-states") {
      options.limit_states = std::stoull(require_value("--limit-states"));
    } else if (arg == "--limit-instructions") {
      options.limit_instructions =
          std::stoull(require_value("--limit-instructions"));
    } else if (arg == "--print-instructions") {
      options.print_instructions = true;
    } else if (!arg.empty() && arg[0] == '-') {
      throw std::runtime_error("unknown option: " + arg);
    } else {
      options.inputs.emplace_back(arg);
    }
  }
  return options;
}

void Require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error("self-test failed: " + message);
  }
}

void RunSelfTests() {
  const auto rax_bytes = ParseHexBytes("04CCCCCCCCCCCCCC");
  Require(rax_bytes.size() == 8, "rax byte count");
  Require(ReadLittleEndianScalar(rax_bytes) == UINT64_C(0xCCCCCCCCCCCCCC04),
          "little-endian register normalization");

  const auto flag_bytes = ParseHexBytes("44000000");
  Require(ReadLittleEndianScalar(flag_bytes) == 0x44,
          "little-endian flags normalization");
  Require(CanonicalStateKey("flags") == "flag", "flags key normalization");
  Require((kUserRFlagsMask & kFlagDF) != 0, "DF is part of user flag mask");
  Require((kUserRFlagsMask & kFlagID) != 0, "ID is part of user flag mask");

  XedDecoder decoder;
  const auto xor_meta = decoder.Decode(0x1000, ParseHexBytes("4831D8"));
  Require(xor_meta.ok, "XED decodes xor rax, rbx");
  Require(xor_meta.iclass == "XOR", "XED iclass for xor rax, rbx");
  Require((xor_meta.undefined_flags & kFlagAF) != 0,
          "XED marks AF undefined for XOR");
  Require((xor_meta.comparable_written_flags & kFlagAF) == 0,
          "undefined AF is excluded from comparable mask");

  const auto not_meta = decoder.Decode(0x1000, ParseHexBytes("48F7D1"));
  Require(not_meta.ok, "XED decodes not rcx");
  Require(not_meta.iclass == "NOT", "XED iclass for not rcx");
  Require(not_meta.written_flags == 0, "NOT does not write user-mode RFLAGS");

  GuestMemory memory;
  Require(memory.Map(0x1000, GuestMemory::kPageSize * 2), "map two pages");
  Require(memory.WriteLittleEndian<std::uint32_t>(0x1ffe, 0xAABBCCDDu),
          "cross-page little-endian write");
  std::uint32_t cross_page = 0;
  Require(memory.ReadLittleEndian(0x1ffe, cross_page),
          "cross-page little-endian read");
  Require(cross_page == 0xAABBCCDDu, "cross-page value roundtrip");

  auto *opaque_memory = memory.opaque();
  __remill_write_memory_32(opaque_memory, 0x1100, 0x11223344u);
  Require(__remill_read_memory_32(opaque_memory, 0x1100) == 0x11223344u,
          "Remill memory helpers read back written values");

  std::uint32_t expected = 0x11223344u;
  __remill_compare_exchange_memory_32(opaque_memory, 0x1100, expected,
                                      0x55667788u);
  Require(expected == 0x11223344u,
          "compare-exchange preserves matching expected");
  Require(__remill_read_memory_32(opaque_memory, 0x1100) == 0x55667788u,
          "compare-exchange writes desired value on match");

  std::uint32_t addend = 2;
  __remill_fetch_and_add_32(opaque_memory, 0x1100, addend);
  Require(addend == 0x55667788u, "fetch-add returns old value by reference");
  Require(__remill_read_memory_32(opaque_memory, 0x1100) == 0x5566778Au,
          "fetch-add stores updated value");

  memory.ClearStatus();
  (void)__remill_read_memory_8(opaque_memory, 0x9000);
  Require(!memory.ok(), "unmapped helper read records a memory fault");
  Require(memory.last_fault()->kind == MemoryFaultKind::Unmapped,
          "unmapped helper read records unmapped fault kind");

  State state;
  ResetState(state);
  Require(SetScalarRegister(state, "rax", 0x1234), "set rax");
  Require(SetScalarRegister(state, "r15", 0x5678), "set r15");
  Require(SetScalarRegister(state, "rip", 0x4000001), "set rip");
  Require(GetScalarRegister(state, "rax") == 0x1234, "get rax");
  Require(GetScalarRegister(state, "r15") == 0x5678, "get r15");
  Require(GetScalarRegister(state, "rip") == 0x4000001, "get rip");

  const auto user_flags = static_cast<std::uint64_t>(
      kFlagCF | kFlagPF | kFlagAF | kFlagZF | kFlagSF | kFlagTF | kFlagIF |
      kFlagDF | kFlagOF | kFlagIOPL | kFlagNT | kFlagRF | kFlagAC | kFlagID);
  SetFlags(state, user_flags);
  Require((GetFlags(state) & kUserRFlagsMask) == user_flags,
          "pack/unpack all user-mode RFLAGS bits");

  std::map<std::string, std::uint64_t> initial_state = {
      {"rax", 0xAAAAAAAAAAAAAAAAull}, {"flag", kFlagCF | kFlagDF}};
  std::string apply_error;
  Require(ApplyInitialState(state, initial_state, &apply_error),
          "apply sparse initial state");
  const auto snapshot = SnapshotState(state, {"rax", "flag"});
  Require(snapshot.at("rax") == 0xAAAAAAAAAAAAAAAAull, "snapshot rax");
  Require((snapshot.at("flag") & (kFlagCF | kFlagDF)) == (kFlagCF | kFlagDF),
          "snapshot flags");

  ExpectationRow xor_row;
  xor_row.address = 0x4000001;
  xor_row.opcode = "4831D8";
  xor_row.instruction = "xor rax, rbx";
  xor_row.initial_state = {{"rax", 0xff}, {"rbx", 0x0f}, {"flag", 0}};
  RemillBackend remill_backend;
  const auto remill_result = remill_backend.RunCase(xor_row, {"rax", "flag"});
  Require(remill_result.outcome_class == OutcomeClass::Normal,
          remill_result.backend_error.value_or(
              remill_result.exception_detail.value_or("Remill smoke failed")));
  Require(remill_result.final_state.at("rax") == 0xf0,
          "Remill smoke computes xor rax, rbx");
  Require((remill_result.final_state.at("flag") &
           xor_meta.comparable_written_flags) ==
              (kFlagPF & xor_meta.comparable_written_flags),
          "Remill smoke flags match comparable XED-defined XOR flags");

  std::cout << "self-test: ok\n";
}

void PrintMetadataLine(const ExpectationRow &row, const XedMetadata &metadata) {
  std::cout << row.opcode << " @0x" << std::hex << std::uppercase << row.address
            << std::dec << " " << row.instruction << " xed=";
  if (!metadata.ok) {
    std::cout << "ERROR(" << metadata.error << ")\n";
    return;
  }

  std::cout << metadata.iclass << " category=" << metadata.category
            << " extension=" << metadata.extension << " len=" << metadata.length
            << " flags.read=" << FormatFlagNames(metadata.read_flags)
            << " flags.written=" << FormatFlagNames(metadata.written_flags)
            << " flags.undefined=" << FormatFlagNames(metadata.undefined_flags)
            << " flags.compare="
            << FormatFlagNames(metadata.comparable_written_flags);
  if (!metadata.memory_operands.empty()) {
    std::cout << " memops=" << metadata.memory_operands.size();
  }
  std::cout << '\n';
}

int Run(const Options &options) {
  if (options.help) {
    PrintUsage("remill-tester");
    return 0;
  }
  if (options.self_test) {
    RunSelfTests();
    if (options.inputs.empty()) {
      return 0;
    }
  }
  if (options.inputs.empty()) {
    PrintUsage("remill-tester");
    return 2;
  }

  X86TesterParser parser;
  XedDecoder decoder;
  Summary summary;
  std::map<std::string, XedMetadata> decode_cache;
  std::set<std::string> selected_groups;
  std::set<std::string> printed_decodes;
  bool stop = false;

  for (const auto &path : options.inputs) {
    if (stop) {
      break;
    }

    const auto corpus = parser.ParseFile(path);
    ++summary.files;
    summary.instruction_groups += corpus.instruction_groups;
    summary.rows += corpus.rows.size();
    summary.parse_warnings += corpus.parse_warnings;

    for (const auto &row : corpus.rows) {
      const auto mnemonic = FirstToken(row.instruction);
      if (!options.mnemonics.empty() &&
          options.mnemonics.count(mnemonic) == 0) {
        continue;
      }
      if (!options.opcodes.empty() && options.opcodes.count(row.opcode) == 0) {
        continue;
      }

      const auto group_key = InstructionGroupKey(path, row);
      if (selected_groups.count(group_key) == 0) {
        if (options.limit_instructions != 0 &&
            selected_groups.size() >= options.limit_instructions) {
          stop = true;
          break;
        }
        selected_groups.insert(group_key);
        ++summary.selected_instruction_groups;
        ++summary.groups_by_mnemonic[mnemonic];
      }

      if (options.limit_states != 0 &&
          summary.selected_rows >= options.limit_states) {
        stop = true;
        break;
      }

      const auto decode_key = DecodeKey(row);
      auto it = decode_cache.find(decode_key);
      if (it == decode_cache.end()) {
        auto metadata = decoder.Decode(row.address, ParseHexBytes(row.opcode));
        it = decode_cache.emplace(decode_key, std::move(metadata)).first;
        ++summary.unique_decodes;
        if (!it->second.ok) {
          ++summary.xed_decode_failures;
        } else if (it->second.length != ParseHexBytes(row.opcode).size()) {
          ++summary.xed_length_mismatches;
        }
      }

      const auto &metadata = it->second;
      if (options.print_instructions &&
          printed_decodes.insert(decode_key).second) {
        PrintMetadataLine(row, metadata);
      }

      ++summary.selected_rows;
      ++summary.rows_by_mnemonic[mnemonic];
      for (const auto &name : FlagNamesFromMask(metadata.undefined_flags)) {
        ++summary.undefined_flag_mentions[name];
      }
      if (metadata.ok && !metadata.memory_operands.empty() &&
          row.initial_memory.empty()) {
        ++summary.memory_rows_without_oracle;
      }
    }
  }

  std::cout << "summary.files=" << summary.files << '\n';
  std::cout << "summary.instruction_groups=" << summary.instruction_groups
            << '\n';
  std::cout << "summary.rows=" << summary.rows << '\n';
  std::cout << "summary.selected_instruction_groups="
            << summary.selected_instruction_groups << '\n';
  std::cout << "summary.selected_rows=" << summary.selected_rows << '\n';
  std::cout << "summary.unique_decodes=" << summary.unique_decodes << '\n';
  std::cout << "summary.xed_decode_failures=" << summary.xed_decode_failures
            << '\n';
  std::cout << "summary.xed_length_mismatches=" << summary.xed_length_mismatches
            << '\n';
  std::cout << "summary.memory_rows_without_oracle="
            << summary.memory_rows_without_oracle << '\n';
  std::cout << "summary.parse_warnings=" << summary.parse_warnings << '\n';

  if (!summary.rows_by_mnemonic.empty()) {
    std::cout << "summary.rows_by_mnemonic:";
    for (const auto &[mnemonic, count] : summary.rows_by_mnemonic) {
      std::cout << ' ' << mnemonic << '=' << count;
    }
    std::cout << '\n';
  }

  if (!summary.undefined_flag_mentions.empty()) {
    std::cout << "summary.undefined_flag_mentions:";
    for (const auto &[flag, count] : summary.undefined_flag_mentions) {
      std::cout << ' ' << flag << '=' << count;
    }
    std::cout << '\n';
  }

  return summary.xed_decode_failures == 0 && summary.xed_length_mismatches == 0
             ? 0
             : 1;
}

} // namespace
} // namespace remill_tester

int main(int argc, char **argv) {
  try {
    const auto options = remill_tester::ParseOptions(argc, argv);
    return remill_tester::Run(options);
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
}
