#include "x86tester_parser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
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
  std::vector<std::string> input_schema;
  std::vector<std::string> output_schema;
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

std::vector<std::string> ParseSchemaField(const std::string &field,
                                          const std::string &name) {
  const auto trimmed = Trim(field);
  const auto prefix = name + "=";
  if (trimmed.rfind(prefix, 0) != 0) {
    throw std::runtime_error("instruction header field must start with " +
                             prefix + ": " + field);
  }

  const auto body = Trim(trimmed.substr(prefix.size()));
  std::vector<std::string> schema;
  if (body.empty()) {
    return schema;
  }

  for (const auto &part : Split(body, ',')) {
    auto key = Trim(part);
    if (key.empty()) {
      throw std::runtime_error("empty state name in " + name + " schema: " +
                               field);
    }
    schema.push_back(std::move(key));
  }
  return schema;
}

InstructionHeader ParseHeader(const std::string &line) {
  constexpr const char *prefix = "instr:";
  if (line.rfind(prefix, 0) != 0) {
    throw std::runtime_error("instruction header does not start with instr:");
  }

  const auto body = line.substr(std::char_traits<char>::length(prefix));
  const auto parts = Split(body, ';');
  if (parts.size() != 6) {
    throw std::runtime_error(
        "instruction header must have six fields: " + line);
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
  header.input_schema = ParseSchemaField(parts[4], "in");
  header.output_schema = ParseSchemaField(parts[5], "out");
  return header;
}

std::uint64_t ParseDataHeader(const std::string &line) {
  constexpr const char *prefix = "data:";
  if (line.rfind(prefix, 0) != 0) {
    throw std::runtime_error("file must start with data:<count>");
  }
  return ParseUnsigned(Trim(line.substr(std::char_traits<char>::length(prefix))),
                       10);
}

std::vector<std::uint8_t> ParseDataPoolValue(const std::string &line) {
  const auto trimmed = Trim(line);
  if (trimmed.empty() || trimmed[0] != '#') {
    throw std::runtime_error("data pool value must start with #: " + line);
  }
  return ParseHexBytes(trimmed.substr(1));
}

bool IsByteStateKey(const std::string &key) {
  const auto has_numeric_suffix = [&](const char *prefix) {
    const auto prefix_size = std::char_traits<char>::length(prefix);
    return key.size() > prefix_size &&
           key.compare(0, prefix_size, prefix) == 0 &&
           std::isdigit(static_cast<unsigned char>(key[prefix_size]));
  };
  return has_numeric_suffix("xmm") || has_numeric_suffix("ymm") ||
         has_numeric_suffix("zmm") || has_numeric_suffix("mm") ||
         has_numeric_suffix("st");
}

void AddStateBytes(
    const std::string &raw_key, const std::vector<std::uint8_t> &bytes,
    std::map<std::string, std::uint64_t> &scalars,
    std::map<std::string, std::vector<std::uint8_t>> &bytes_map) {
  auto key = CanonicalStateKey(raw_key);
  if (!bytes.empty() && bytes.size() <= 8) {
    scalars[key] = ReadLittleEndianScalar(bytes);
  }
  if (IsByteStateKey(key) || bytes.size() > 8) {
    bytes_map[key] = bytes;
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

void AddPooledValue(
    const std::string &raw_key, const std::vector<std::uint8_t> &bytes,
    std::map<std::string, std::uint64_t> &scalars,
    std::map<std::string, std::vector<std::uint8_t>> &bytes_map,
    std::vector<MemoryCell> *input_memory = nullptr,
    std::vector<MemoryExpectation> *output_memory = nullptr) {
  if (const auto memory_address = ParseMemoryAddressKey(raw_key)) {
    auto memory_bytes = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
    if (input_memory != nullptr) {
      AddMemoryInput(*input_memory, *memory_address, std::move(memory_bytes));
    } else if (output_memory != nullptr) {
      AddMemoryOutput(*output_memory, *memory_address, std::move(memory_bytes));
    } else {
      throw std::runtime_error("memory key is not valid here: " + raw_key);
    }
    return;
  }

  AddStateBytes(raw_key, bytes, scalars, bytes_map);
}

std::vector<std::size_t>
ParseIndexList(const std::string &text, std::size_t expected_count,
               std::size_t data_pool_size, const std::string &kind) {
  const auto trimmed = Trim(text);
  if (expected_count == 0) {
    if (!trimmed.empty()) {
      throw std::runtime_error(kind +
                               " row has values but schema is empty: " + text);
    }
    return {};
  }
  if (trimmed.empty()) {
    throw std::runtime_error(kind + " row is empty but schema is not");
  }

  const auto parts = Split(trimmed, ',');
  if (parts.size() != expected_count) {
    std::ostringstream msg;
    msg << kind << " row has " << parts.size() << " values but schema has "
        << expected_count << ": " << text;
    throw std::runtime_error(msg.str());
  }

  std::vector<std::size_t> indices;
  indices.reserve(parts.size());
  for (const auto &part : parts) {
    const auto index_text = Trim(part);
    if (index_text.empty()) {
      throw std::runtime_error("empty " + kind + " data pool index: " + text);
    }
    const auto raw_index = ParseUnsigned(index_text, 10);
    if (raw_index >= data_pool_size) {
      std::ostringstream msg;
      msg << kind << " data pool index " << raw_index
          << " is out of range for pool size " << data_pool_size;
      throw std::runtime_error(msg.str());
    }
    if (raw_index > static_cast<std::uint64_t>(
                        std::numeric_limits<std::size_t>::max())) {
      throw std::runtime_error(kind + " data pool index is too large: " +
                               index_text);
    }
    indices.push_back(static_cast<std::size_t>(raw_index));
  }
  return indices;
}

void ApplySchemaValues(
    const std::vector<std::string> &schema,
    const std::vector<std::size_t> &indices,
    const std::vector<std::vector<std::uint8_t>> &data_pool,
    std::map<std::string, std::uint64_t> &scalars,
    std::map<std::string, std::vector<std::uint8_t>> &bytes_map,
    std::vector<MemoryCell> *input_memory = nullptr,
    std::vector<MemoryExpectation> *output_memory = nullptr) {
  if (schema.size() != indices.size()) {
    throw std::runtime_error("schema/value count mismatch");
  }
  for (std::size_t i = 0; i < schema.size(); ++i) {
    AddPooledValue(schema[i], data_pool[indices[i]], scalars, bytes_map,
                   input_memory, output_memory);
  }
}

ExpectationRow
ParseStateRow(const std::string &line, const InstructionHeader &header,
              std::uint64_t instruction_id, std::uint64_t test_case_id,
              std::uint64_t state_index,
              const std::vector<std::vector<std::uint8_t>> &data_pool) {
  const auto trimmed = Trim(line);
  const auto separator = trimmed.find('|');
  if (separator == std::string::npos) {
    throw std::runtime_error("state row must contain |: " + line);
  }
  if (trimmed.find('|', separator + 1) != std::string::npos) {
    throw std::runtime_error("state row must contain exactly one |: " + line);
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

  const auto input_indices = ParseIndexList(
      trimmed.substr(0, separator), header.input_schema.size(),
      data_pool.size(), "input");
  ApplySchemaValues(header.input_schema, input_indices, data_pool,
                    row.initial_state, row.initial_bytes,
                    &row.initial_memory, nullptr);

  std::map<std::string, std::uint64_t> expected_scalars;
  std::map<std::string, std::vector<std::uint8_t>> expected_bytes;

  const auto output_text = Trim(trimmed.substr(separator + 1));
  if (output_text.rfind("!", 0) == 0) {
    auto exception_kind = Trim(output_text.substr(1));
    if (exception_kind.empty()) {
      throw std::runtime_error("exception output is missing an exception kind: " +
                               line);
    }
    row.expected_exception_kind = std::move(exception_kind);
  } else {
    const auto output_indices = ParseIndexList(
        output_text, header.output_schema.size(), data_pool.size(), "output");
    ApplySchemaValues(header.output_schema, output_indices, data_pool,
                      expected_scalars, expected_bytes, nullptr,
                      &row.expected_memory);
  }

  row.expected_final_state = expected_scalars;
  row.expected_final_bytes = expected_bytes;
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

namespace {

int HexDigitValue(unsigned char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  return -1;
}

} // namespace

std::vector<std::uint8_t> ParseHexBytes(const std::string &hex) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(hex.size() / 2);

  int high_nibble = -1;
  for (const auto raw_ch : hex) {
    const auto ch = static_cast<unsigned char>(raw_ch);
    if (std::isspace(ch) || ch == '_') {
      continue;
    }
    const auto digit = HexDigitValue(ch);
    if (digit < 0) {
      throw std::runtime_error("invalid hex byte string: " + hex);
    }
    if (high_nibble < 0) {
      high_nibble = digit;
    } else {
      bytes.push_back(static_cast<std::uint8_t>((high_nibble << 4) | digit));
      high_nibble = -1;
    }
  }

  if (high_nibble >= 0) {
    throw std::runtime_error("hex byte string must have even length: " + hex);
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

std::vector<ParsedInstructionHeader>
X86TesterParser::ParseInstructionHeaders(const std::filesystem::path &path) const {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open input file: " + path.string());
  }

  std::vector<ParsedInstructionHeader> headers;
  std::string line;
  std::uint64_t line_number = 0;
  std::uint64_t instruction_id = 0;
  while (std::getline(input, line)) {
    ++line_number;
    const auto trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.rfind("instr:", 0) != 0) {
      continue;
    }

    try {
      const auto header = ParseHeader(trimmed);
      ParsedInstructionHeader parsed;
      parsed.instruction_id = ++instruction_id;
      parsed.address = header.address;
      parsed.opcode = header.opcode;
      parsed.instruction = header.instruction;
      parsed.row_count = header.row_count;
      headers.push_back(std::move(parsed));
    } catch (const std::exception &e) {
      std::ostringstream msg;
      msg << path << ':' << line_number << ": " << e.what();
      throw std::runtime_error(msg.str());
    }
  }
  return headers;
}

ParsedCorpus
X86TesterParser::ParseFile(const std::filesystem::path &path) const {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open input file: " + path.string());
  }

  ParsedCorpus corpus;
  std::vector<std::vector<std::uint8_t>> data_pool;
  std::string line;
  std::uint64_t line_number = 0;
  std::uint64_t data_values_remaining = 0;
  std::uint64_t instruction_id = 0;
  std::uint64_t next_test_case_id = 0;
  std::uint64_t state_index = 0;
  std::uint64_t rows_in_group = 0;
  InstructionHeader current_header;
  bool have_data_header = false;
  bool have_header = false;

  const auto finish_group = [&]() {
    if (have_header && rows_in_group != current_header.row_count) {
      ++corpus.parse_warnings;
    }
  };

  while (std::getline(input, line)) {
    ++line_number;
    const auto trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }

    try {
      if (!have_data_header) {
        data_values_remaining = ParseDataHeader(trimmed);
        have_data_header = true;
        data_pool.reserve(static_cast<std::size_t>(data_values_remaining));
        continue;
      }

      if (data_values_remaining != 0) {
        data_pool.push_back(ParseDataPoolValue(trimmed));
        --data_values_remaining;
        continue;
      }

      if (trimmed.rfind("data:", 0) == 0) {
        throw std::runtime_error("duplicate data pool header");
      }

      if (trimmed.rfind("instr:", 0) == 0) {
        finish_group();
        current_header = ParseHeader(trimmed);
        have_header = true;
        state_index = 0;
        rows_in_group = 0;
        ++instruction_id;
        ++corpus.instruction_groups;
      } else {
        if (!have_header) {
          throw std::runtime_error("state row appears before first header");
        }
        corpus.rows.push_back(ParseStateRow(trimmed, current_header,
                                            instruction_id, next_test_case_id++,
                                            state_index, data_pool));
        ++state_index;
        ++rows_in_group;
      }
    } catch (const std::exception &e) {
      std::ostringstream msg;
      msg << path << ':' << line_number << ": " << e.what();
      throw std::runtime_error(msg.str());
    }
  }

  if (!have_data_header) {
    throw std::runtime_error(path.string() + ": missing data pool header");
  }
  if (data_values_remaining != 0) {
    std::ostringstream msg;
    msg << path << ": expected " << data_values_remaining
        << " more data pool values";
    throw std::runtime_error(msg.str());
  }

  finish_group();
  return corpus;
}

} // namespace remill_tester
