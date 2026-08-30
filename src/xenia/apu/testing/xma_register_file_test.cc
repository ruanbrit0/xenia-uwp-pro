/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <string>

#include "third_party/catch/include/catch.hpp"
#include "xenia/apu/xma_register_file.h"

namespace xe::apu::test {

TEST_CASE("XMA register file starts cleared", "[apu]") {
  XmaRegisterFile registers;

  REQUIRE(registers[XmaRegister::ContextArrayAddress] == 0);
  REQUIRE(registers[XmaRegister::CurrentContextIndex] == 0);
  REQUIRE(registers[XmaRegister::NextContextIndex] == 0);
  REQUIRE(registers[XmaRegister::Context0Kick] == 0);
}

TEST_CASE("XMA register file exposes known register names", "[apu]") {
  const auto* context_array_address =
      XmaRegisterFile::GetRegisterInfo(XmaRegister::ContextArrayAddress);
  REQUIRE(context_array_address != nullptr);
  REQUIRE(std::string(context_array_address->name) == "ContextArrayAddress");

  const auto* context_0_kick =
      XmaRegisterFile::GetRegisterInfo(XmaRegister::Context0Kick);
  REQUIRE(context_0_kick != nullptr);
  REQUIRE(std::string(context_0_kick->name) == "Context0Kick");

  REQUIRE(XmaRegisterFile::GetRegisterInfo(0x0601) == nullptr);
}

TEST_CASE("XMA register file stores independent values", "[apu]") {
  XmaRegisterFile registers;

  registers[XmaRegister::ContextArrayAddress] = 0x12345678;
  registers[XmaRegister::CurrentContextIndex] = 3;
  registers[XmaRegister::NextContextIndex] = 4;
  registers[XmaRegister::Context0Kick] = 1;

  REQUIRE(registers[XmaRegister::ContextArrayAddress] == 0x12345678);
  REQUIRE(registers[XmaRegister::CurrentContextIndex] == 3);
  REQUIRE(registers[XmaRegister::NextContextIndex] == 4);
  REQUIRE(registers[XmaRegister::Context0Kick] == 1);
}

}  // namespace xe::apu::test
