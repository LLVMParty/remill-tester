#include "xed_metadata.hpp"

#include "flag_masks.hpp"

#include <mutex>
#include <sstream>

extern "C" {
#include <xed/xed-interface.h>
}

namespace remill_tester {
namespace {

void InitXedTablesOnce() {
  static std::once_flag once;
  std::call_once(once, [] { xed_tables_init(); });
}

std::string SafeString(const char *text) {
  return text ? std::string(text) : std::string();
}

} // namespace

XedDecoder::XedDecoder() { InitXedTablesOnce(); }

XedMetadata XedDecoder::Decode(std::uint64_t address,
                               const std::vector<std::uint8_t> &bytes) const {
  InitXedTablesOnce();

  XedMetadata metadata;
  if (bytes.empty() || bytes.size() > 15) {
    metadata.error = "opcode length must be between 1 and 15 bytes";
    return metadata;
  }

  xed_decoded_inst_t xedd;
  xed_decoded_inst_zero(&xedd);
  xed_decoded_inst_set_mode(&xedd, XED_MACHINE_MODE_LONG_64,
                            XED_ADDRESS_WIDTH_64b);
  xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_ALL);

  const auto error = xed_decode(&xedd, bytes.data(), bytes.size());
  if (error != XED_ERROR_NONE) {
    metadata.error = SafeString(xed_error_enum_t2str(error));
    return metadata;
  }

  metadata.ok = true;
  metadata.length = xed_decoded_inst_get_length(&xedd);
  metadata.operand_width = xed_decoded_inst_get_operand_width(&xedd);
  metadata.iclass =
      SafeString(xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&xedd)));
  metadata.iform =
      SafeString(xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(&xedd)));
  metadata.category =
      SafeString(xed_category_enum_t2str(xed_decoded_inst_get_category(&xedd)));
  metadata.extension = SafeString(
      xed_extension_enum_t2str(xed_decoded_inst_get_extension(&xedd)));

  if (const auto *flags = xed_decoded_inst_get_rflags_info(&xedd)) {
    if (const auto *read = xed_simple_flag_get_read_flag_set(flags)) {
      metadata.read_flags = read->flat & kUserRFlagsMask;
    }
    if (const auto *written = xed_simple_flag_get_written_flag_set(flags)) {
      metadata.written_flags = written->flat & kUserRFlagsMask;
    }
    if (const auto *undefined = xed_simple_flag_get_undefined_flag_set(flags)) {
      metadata.undefined_flags = undefined->flat & kUserRFlagsMask;
    }
    metadata.comparable_written_flags =
        metadata.written_flags & ~metadata.undefined_flags & kUserRFlagsMask;
  }

  const auto memory_operand_count =
      xed_decoded_inst_number_of_memory_operands(&xedd);
  metadata.memory_operands.reserve(memory_operand_count);
  for (unsigned i = 0; i < memory_operand_count; ++i) {
    XedMemoryOperand mem;
    mem.index = i;
    mem.length = xed_decoded_inst_get_memory_operand_length(&xedd, i);
    mem.address_width = xed_decoded_inst_get_memop_address_width(&xedd, i);
    mem.reads = xed_decoded_inst_mem_read(&xedd, i) != 0;
    mem.writes = xed_decoded_inst_mem_written(&xedd, i) != 0;
    mem.writes_only = xed_decoded_inst_mem_written_only(&xedd, i) != 0;
    metadata.memory_operands.push_back(mem);
  }

  (void)address;
  return metadata;
}

} // namespace remill_tester
