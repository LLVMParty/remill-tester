#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace remill_tester {

constexpr std::uint32_t kFlagCF = 1u << 0;
constexpr std::uint32_t kFlagPF = 1u << 2;
constexpr std::uint32_t kFlagAF = 1u << 4;
constexpr std::uint32_t kFlagZF = 1u << 6;
constexpr std::uint32_t kFlagSF = 1u << 7;
constexpr std::uint32_t kFlagTF = 1u << 8;
constexpr std::uint32_t kFlagIF = 1u << 9;
constexpr std::uint32_t kFlagDF = 1u << 10;
constexpr std::uint32_t kFlagOF = 1u << 11;
constexpr std::uint32_t kFlagIOPL = (1u << 12) | (1u << 13);
constexpr std::uint32_t kFlagNT = 1u << 14;
constexpr std::uint32_t kFlagRF = 1u << 16;
constexpr std::uint32_t kFlagAC = 1u << 18;
constexpr std::uint32_t kFlagID = 1u << 21;

constexpr std::uint32_t kUserRFlagsMask =
    kFlagCF | kFlagPF | kFlagAF | kFlagZF | kFlagSF | kFlagTF | kFlagIF |
    kFlagDF | kFlagOF | kFlagIOPL | kFlagNT | kFlagRF | kFlagAC | kFlagID;

constexpr std::uint32_t kStatusRFlagsMask =
    kFlagCF | kFlagPF | kFlagAF | kFlagZF | kFlagSF | kFlagOF;

struct FlagBitInfo {
  std::uint32_t mask;
  const char *name;
};

const std::vector<FlagBitInfo> &UserFlagBits();
std::vector<std::string> FlagNamesFromMask(std::uint32_t mask);
std::string FormatFlagNames(std::uint32_t mask);

} // namespace remill_tester
