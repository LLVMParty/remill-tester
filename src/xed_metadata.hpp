#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace remill_tester {

struct XedMemoryOperand {
  unsigned index = 0;
  unsigned length = 0;
  unsigned address_width = 0;
  bool reads = false;
  bool writes = false;
  bool writes_only = false;
};

struct XedMetadata {
  bool ok = false;
  std::string error;
  unsigned length = 0;
  unsigned operand_width = 0;
  std::string iclass;
  std::string category;
  std::string extension;
  std::uint32_t read_flags = 0;
  std::uint32_t written_flags = 0;
  std::uint32_t undefined_flags = 0;
  std::uint32_t comparable_written_flags = 0;
  std::vector<XedMemoryOperand> memory_operands;
};

class XedDecoder {
public:
  XedDecoder();

  XedMetadata Decode(std::uint64_t address,
                     const std::vector<std::uint8_t> &bytes) const;
};

} // namespace remill_tester
