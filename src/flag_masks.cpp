#include "flag_masks.hpp"

#include <sstream>

namespace remill_tester {

const std::vector<FlagBitInfo> &UserFlagBits() {
  static const std::vector<FlagBitInfo> bits = {
      {kFlagCF, "CF"},       {kFlagPF, "PF"},       {kFlagAF, "AF"},
      {kFlagZF, "ZF"},       {kFlagSF, "SF"},       {kFlagTF, "TF"},
      {kFlagIF, "IF"},       {kFlagDF, "DF"},       {kFlagOF, "OF"},
      {1u << 12, "IOPL[0]"}, {1u << 13, "IOPL[1]"}, {kFlagNT, "NT"},
      {kFlagRF, "RF"},       {kFlagAC, "AC"},       {kFlagID, "ID"},
  };
  return bits;
}

std::vector<std::string> FlagNamesFromMask(std::uint32_t mask) {
  std::vector<std::string> names;
  for (const auto &bit : UserFlagBits()) {
    if ((mask & bit.mask) != 0) {
      names.emplace_back(bit.name);
    }
  }
  return names;
}

std::string FormatFlagNames(std::uint32_t mask) {
  const auto names = FlagNamesFromMask(mask);
  if (names.empty()) {
    return "-";
  }

  std::ostringstream out;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    out << names[i];
  }
  return out.str();
}

} // namespace remill_tester
