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
#include "xenia/apu/xma_helpers.h"

namespace xe::apu::xma::test {

TEST_CASE("XMA packet helpers parse metadata", "[apu]") {
  uint8_t packet[4] = {};

  packet[0] = 5 << 2;
  packet[2] = 1;
  packet[3] = 7;

  REQUIRE(GetPacketFrameCount(packet) == 5);
  REQUIRE(GetPacketMetadata(packet) == 1);
  REQUIRE(IsPacketXma2Type(packet));
  REQUIRE(GetPacketSkipCount(packet) == 7);

  packet[2] = 4;
  REQUIRE(GetPacketMetadata(packet) == 4);
  REQUIRE_FALSE(IsPacketXma2Type(packet));

  packet[2] = 0xF9;
  REQUIRE(GetPacketMetadata(packet) == 1);
  REQUIRE(IsPacketXma2Type(packet));
}

TEST_CASE("XMA packet helpers parse first frame bit offset", "[apu]") {
  uint8_t packet[4] = {};

  packet[0] = 0x03;
  packet[1] = 0xFF;
  packet[2] = 0xF8;
  REQUIRE(GetPacketFrameOffset(packet) == kMaxFrameLength + 32);

  packet[0] = 0x00;
  packet[1] = 0x00;
  packet[2] = 0x00;
  REQUIRE(GetPacketFrameOffset(packet) == 32);

  packet[0] = 0x02;
  packet[1] = 0x34;
  packet[2] = 0x58;
  const uint32_t expected_offset =
      (2u << 13) | (0x34u << 5) | (0x58u >> 3);
  REQUIRE(GetPacketFrameOffset(packet) == expected_offset + 32);
}

TEST_CASE("XMA packet helpers separate frame count and offset bits", "[apu]") {
  uint8_t packet[4] = {};

  packet[0] = (9 << 2) | 0x03;
  packet[1] = 0x01;
  packet[2] = 0x08;

  REQUIRE(GetPacketFrameCount(packet) == 9);
  REQUIRE(GetPacketFrameOffset(packet) == ((3u << 13) | (1u << 5) | 1u) + 32);
}

}  // namespace xe::apu::xma::test
