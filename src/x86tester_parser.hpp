#pragma once

#include "expectation.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace remill_tester {

struct ParsedCorpus {
  std::vector<ExpectationRow> rows;
  std::uint64_t instruction_groups = 0;
  std::uint64_t parse_warnings = 0;
};

std::string ToLower(std::string value);
std::string ToUpper(std::string value);
std::string Trim(std::string value);
std::vector<std::uint8_t> ParseHexBytes(const std::string &hex);
std::uint64_t ReadLittleEndianScalar(const std::vector<std::uint8_t> &bytes);
std::string CanonicalStateKey(std::string key);
std::optional<std::uint64_t> ParseMemoryAddressKey(const std::string &key);

class X86TesterParser {
public:
  ParsedCorpus ParseFile(const std::filesystem::path &path) const;
};

} // namespace remill_tester
