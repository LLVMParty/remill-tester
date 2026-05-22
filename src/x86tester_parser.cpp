#include "x86tester_parser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace remill_tester {
namespace {

struct InstructionHeader {
  std::uint64_t address = 0;
  std::string opcode;
  std::string instruction;
  std::uint64_t row_count = 0;
};

std::vector<std::string> Split(const std::string &text, char delimiter) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto pos = text.find(delimiter, start);
    if (pos == std::string::npos) {
      parts.push_back(text.substr(start));
      break;
    }
    parts.push_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return parts;
}

std::uint64_t ParseUnsigned(const std::string &text, int base = 0) {
  std::size_t consumed = 0;
  const auto value = std::stoull(text, &consumed, base);
  if (consumed != text.size()) {
    throw std::runtime_error("invalid integer: " + text);
  }
  return value;
}

InstructionHeader ParseHeader(const std::string &line) {
  constexpr const char *prefix = "instr:";
  if (line.rfind(prefix, 0) != 0) {
    throw std::runtime_error("instruction header does not start with instr:");
  }

  const auto body = line.substr(std::char_traits<char>::length(prefix));
  const auto parts = Split(body, ';');
  if (parts.size() != 4) {
    throw std::runtime_error("instruction header must have four fields: " +
                             line);
  }
  if (parts[1].empty() || parts[1][0] != '#') {
    throw std::runtime_error(
        "instruction header opcode field must start with #: " + line);
  }

  InstructionHeader header;
  header.address = ParseUnsigned(Trim(parts[0]), 0);
  const auto opcode_field = Trim(parts[1]);
  if (opcode_field.empty() || opcode_field[0] != '#') {
    throw std::runtime_error(
        "instruction header opcode field must start with #: " + line);
  }
  header.opcode = ToUpper(opcode_field.substr(1));
  (void)ParseHexBytes(header.opcode);
  header.instruction = Trim(parts[2]);
  header.row_count = ParseUnsigned(Trim(parts[3]), 10);
  return header;
}

void AddStateValue(
    const std::string &raw_key, const std::string &raw_hex,
    std::map<std::string, std::uint64_t> &scalars,
    std::map<std::string, std::vector<std::uint8_t>> &bytes_map) {
  auto key = CanonicalStateKey(raw_key);
  auto bytes = ParseHexBytes(raw_hex);
  bytes_map[key] = bytes;
  if (!bytes.empty() && bytes.size() <= 8) {
    scalars[key] = ReadLittleEndianScalar(bytes);
  }
}

void AddMemoryInput(std::vector<MemoryCell> &memory, std::uint64_t address,
                    std::vector<std::uint8_t> bytes) {
  MemoryCell cell;
  cell.address = address;
  cell.bytes = std::move(bytes);
  cell.permissions = 0;
  memory.push_back(std::move(cell));
}

void AddMemoryOutput(std::vector<MemoryExpectation> &memory,
                     std::uint64_t address, std::vector<std::uint8_t> bytes) {
  MemoryExpectation expectation;
  expectation.address = address;
  expectation.bytes = std::move(bytes);
  memory.push_back(std::move(expectation));
}

void ParseStateMap(const std::string &text,
                   std::map<std::string, std::uint64_t> &scalars,
                   std::map<std::string, std::vector<std::uint8_t>> &bytes_map,
                   std::vector<MemoryCell> *input_memory = nullptr,
                   std::vector<MemoryExpectation> *output_memory = nullptr) {
  const auto trimmed = Trim(text);
  if (trimmed.empty()) {
    return;
  }

  for (const auto &entry_text : Split(trimmed, ',')) {
    const auto entry = Trim(entry_text);
    if (entry.empty()) {
      continue;
    }

    const auto sep = entry.find(":#");
    if (sep == std::string::npos) {
      throw std::runtime_error("state entry must be key:#hex: " + entry);
    }
    const auto key = entry.substr(0, sep);
    const auto hex = entry.substr(sep + 2);
    if (const auto memory_address = ParseMemoryAddressKey(key)) {
      auto bytes = ParseHexBytes(hex);
      if (input_memory != nullptr) {
        AddMemoryInput(*input_memory, *memory_address, std::move(bytes));
      } else if (output_memory != nullptr) {
        AddMemoryOutput(*output_memory, *memory_address, std::move(bytes));
      } else {
        throw std::runtime_error("memory key is not valid in this state map: " +
                                 key);
      }
      continue;
    }
    AddStateValue(key, hex, scalars, bytes_map);
  }
}

ExpectationRow ParseStateRow(const std::string &line,
                             const InstructionHeader &header,
                             std::uint64_t instruction_id,
                             std::uint64_t test_case_id,
                             std::uint64_t state_index) {
  auto trimmed = Trim(line);
  if (trimmed.rfind("in:", 0) == 0) {
    const auto out_pos = trimmed.find("out:", 3);
    if (out_pos != std::string::npos &&
        (out_pos == 0 || trimmed[out_pos - 1] != '|')) {
      trimmed.insert(out_pos, "|");
    }
  }
  if (trimmed.rfind("in:", 0) != 0) {
    throw std::runtime_error("state row must start with in:: " + line);
  }

  ExpectationRow row;
  row.test_case_id = test_case_id;
  row.instruction_id = instruction_id;
  row.address = header.address;
  row.instruction = header.instruction;
  row.opcode = header.opcode;
  row.state_index = state_index;
  row.total_states = header.row_count;
  row.selected_states = header.row_count;
  row.selected_state_index = state_index;

  std::map<std::string, std::uint64_t> expected_scalars;
  std::map<std::string, std::vector<std::uint8_t>> expected_bytes;

  for (const auto &segment_text : Split(trimmed, '|')) {
    const auto segment = Trim(segment_text);
    if (segment.rfind("in:", 0) == 0) {
      ParseStateMap(segment.substr(3), row.initial_state, row.initial_bytes,
                    &row.initial_memory, nullptr);
    } else if (segment.rfind("out:", 0) == 0) {
      ParseStateMap(segment.substr(4), expected_scalars, expected_bytes,
                    nullptr, &row.expected_memory);
      row.expected_final_state = expected_scalars;
      row.expected_final_bytes = expected_bytes;
    } else if (segment.rfind("exception:", 0) == 0) {
      auto exception_kind = Trim(segment.substr(10));
      if (!exception_kind.empty() && ToUpper(exception_kind) != "NONE") {
        row.expected_exception_kind = std::move(exception_kind);
      }
    } else if (!segment.empty()) {
      throw std::runtime_error("unknown state row segment: " + segment);
    }
  }

  if (!row.expected_final_state.has_value()) {
    row.expected_final_state = expected_scalars;
    row.expected_final_bytes = expected_bytes;
  }

  return row;
}

} // namespace

std::string Trim(std::string value) {
  const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
              value.end());
  return value;
}

std::string ToLower(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

std::string ToUpper(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return value;
}

std::vector<std::uint8_t> ParseHexBytes(const std::string &hex) {
  auto clean = Trim(hex);
  clean.erase(std::remove_if(clean.begin(), clean.end(),
                             [](unsigned char ch) {
                               return std::isspace(ch) || ch == '_';
                             }),
              clean.end());
  if (clean.size() % 2 != 0) {
    throw std::runtime_error("hex byte string must have even length: " + hex);
  }

  std::vector<std::uint8_t> bytes;
  bytes.reserve(clean.size() / 2);
  for (std::size_t i = 0; i < clean.size(); i += 2) {
    unsigned int value = 0;
    const auto pair = clean.substr(i, 2);
    auto *begin = pair.data();
    auto *end = pair.data() + pair.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value, 16);
    if (ec != std::errc{} || ptr != end || value > 0xff) {
      throw std::runtime_error("invalid hex byte: " + pair);
    }
    bytes.push_back(static_cast<std::uint8_t>(value));
  }
  return bytes;
}

std::uint64_t ReadLittleEndianScalar(const std::vector<std::uint8_t> &bytes) {
  if (bytes.size() > 8) {
    throw std::runtime_error("little-endian scalar is limited to 8 bytes");
  }
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
  }
  return value;
}

std::string CanonicalStateKey(std::string key) {
  key = ToLower(Trim(std::move(key)));
  if (key == "flags" || key == "eflags" || key == "rflags") {
    return "flag";
  }
  return key;
}

std::optional<std::uint64_t> ParseMemoryAddressKey(const std::string &raw_key) {
  const auto key = ToLower(Trim(raw_key));
  std::string address_text;
  if (key.rfind("mem[", 0) == 0 && !key.empty() && key.back() == ']') {
    address_text = key.substr(4, key.size() - 5);
  } else if (key.rfind("mem@", 0) == 0) {
    address_text = key.substr(4);
  } else {
    return std::nullopt;
  }
  if (address_text.empty()) {
    throw std::runtime_error("memory key has empty address: " + raw_key);
  }
  return ParseUnsigned(address_text, 0);
}

ParsedCorpus
X86TesterParser::ParseFile(const std::filesystem::path &path) const {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open input file: " + path.string());
  }

  ParsedCorpus corpus;
  std::string line;
  std::uint64_t line_number = 0;
  std::uint64_t instruction_id = 0;
  std::uint64_t next_test_case_id = 0;
  std::uint64_t state_index = 0;
  std::uint64_t rows_in_group = 0;
  InstructionHeader current_header;
  bool have_header = false;

  const auto finish_group = [&]() {
    if (have_header && rows_in_group != current_header.row_count) {
      ++corpus.parse_warnings;
    }
  };

  while (std::getline(input, line)) {
    ++line_number;
    if (Trim(line).empty()) {
      continue;
    }

    try {
      if (line.rfind("instr:", 0) == 0) {
        finish_group();
        current_header = ParseHeader(line);
        have_header = true;
        state_index = 0;
        rows_in_group = 0;
        ++instruction_id;
        ++corpus.instruction_groups;
      } else {
        if (!have_header) {
          throw std::runtime_error("state row appears before first header");
        }
        corpus.rows.push_back(ParseStateRow(line, current_header,
                                            instruction_id, next_test_case_id++,
                                            state_index));
        ++state_index;
        ++rows_in_group;
      }
    } catch (const std::exception &e) {
      std::ostringstream msg;
      msg << path << ':' << line_number << ": " << e.what();
      throw std::runtime_error(msg.str());
    }
  }

  finish_group();
  return corpus;
}

} // namespace remill_tester
