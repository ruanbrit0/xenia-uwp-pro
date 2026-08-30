/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <cstdint>

#include "third_party/catch/include/catch.hpp"
#include "xenia/apu/xma_context.h"

namespace xe::apu::test {

TEST_CASE("XMA context reports current input buffer state", "[apu]") {
  uint32_t raw[16] = {};
  XMA_CONTEXT_DATA context(raw);

  context.input_buffer_0_valid = 1;
  context.input_buffer_1_valid = 0;
  context.current_buffer = 0;
  context.input_buffer_0_ptr = 0x1000;
  context.input_buffer_1_ptr = 0x2000;
  context.input_buffer_0_packet_count = 3;
  context.input_buffer_1_packet_count = 5;

  REQUIRE(context.IsInputBufferValid(0));
  REQUIRE_FALSE(context.IsInputBufferValid(1));
  REQUIRE(context.IsCurrentInputBufferValid());
  REQUIRE(context.IsAnyInputBufferValid());
  REQUIRE(context.GetCurrentInputBufferAddress() == 0x1000);
  REQUIRE(context.GetCurrentInputBufferPacketCount() == 3);

  context.current_buffer = 1;

  REQUIRE_FALSE(context.IsCurrentInputBufferValid());
  REQUIRE(context.IsAnyInputBufferValid());
  REQUIRE(context.GetCurrentInputBufferAddress() == 0x2000);
  REQUIRE(context.GetCurrentInputBufferPacketCount() == 5);
}

TEST_CASE("XMA context store roundtrips big endian guest data", "[apu]") {
  uint32_t raw[16] = {};
  uint32_t stored[16] = {};
  XMA_CONTEXT_DATA context(raw);

  context.input_buffer_0_packet_count = 0x123;
  context.loop_count = 0x45;
  context.input_buffer_0_valid = 1;
  context.output_buffer_block_count = 0x12;
  context.input_buffer_1_packet_count = 0x234;
  context.loop_subframe_start = 2;
  context.loop_subframe_end = 6;
  context.subframe_decode_count = 9;
  context.sample_rate = 2;
  context.is_stereo = 1;
  context.input_buffer_read_offset = 0x123456;
  context.error_status = 0x2A;
  context.input_buffer_0_ptr = 0x11223344;
  context.input_buffer_1_ptr = 0x55667788;
  context.output_buffer_ptr = 0x99AABBCC;
  context.work_buffer_ptr = 0xDDEEFF00;
  context.stop_when_done = 1;
  context.interrupt_when_done = 1;

  context.Store(stored);
  XMA_CONTEXT_DATA reloaded(stored);

  REQUIRE(reloaded.input_buffer_0_packet_count == 0x123);
  REQUIRE(reloaded.loop_count == 0x45);
  REQUIRE(reloaded.input_buffer_0_valid == 1);
  REQUIRE(reloaded.output_buffer_block_count == 0x12);
  REQUIRE(reloaded.input_buffer_1_packet_count == 0x234);
  REQUIRE(reloaded.loop_subframe_start == 2);
  REQUIRE(reloaded.loop_subframe_end == 6);
  REQUIRE(reloaded.subframe_decode_count == 9);
  REQUIRE(reloaded.sample_rate == 2);
  REQUIRE(reloaded.is_stereo == 1);
  REQUIRE(reloaded.input_buffer_read_offset == 0x123456);
  REQUIRE(reloaded.error_status == 0x2A);
  REQUIRE(reloaded.input_buffer_0_ptr == 0x11223344);
  REQUIRE(reloaded.input_buffer_1_ptr == 0x55667788);
  REQUIRE(reloaded.output_buffer_ptr == 0x99AABBCC);
  REQUIRE(reloaded.work_buffer_ptr == 0xDDEEFF00);
  REQUIRE(reloaded.stop_when_done == 1);
  REQUIRE(reloaded.interrupt_when_done == 1);
}

}  // namespace xe::apu::test
