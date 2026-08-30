/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <cstring>

#include "third_party/catch/include/catch.hpp"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/testing/kernel_test_util.h"
#include "xenia/kernel/xevent.h"
#include "xenia/memory.h"
#include "xenia/xbox.h"

namespace xe::kernel::test {

namespace {

uint32_t AllocateOverlapped(Memory* memory) {
  uint32_t overlapped_ptr = 0;
  REQUIRE(memory->LookupHeap(0x10000000)
              ->Alloc(sizeof(XAM_OVERLAPPED), 0x10,
                      kMemoryAllocationCommit | kMemoryAllocationReserve,
                      kMemoryProtectRead | kMemoryProtectWrite, false,
                      &overlapped_ptr));
  std::memset(memory->TranslateVirtual(overlapped_ptr), 0,
              sizeof(XAM_OVERLAPPED));
  return overlapped_ptr;
}

}  // namespace

TEST_CASE("Kernel overlapped completion writes result fields", "[kernel]") {
  KernelStateFixture fixture;
  auto* memory = fixture.memory();
  auto* kernel_state = fixture.kernel_state();
  const uint32_t overlapped_ptr = AllocateOverlapped(memory);

  kernel_state->CompleteOverlappedEx(overlapped_ptr, X_ERROR_ACCESS_DENIED,
                                     X_ERROR_FILE_NOT_FOUND, 0x1234);

  auto overlapped = memory->TranslateVirtual(overlapped_ptr);
  REQUIRE(XOverlappedGetResult(overlapped) == X_ERROR_ACCESS_DENIED);
  REQUIRE(XOverlappedGetLength(overlapped) == 0x1234);
  auto words = reinterpret_cast<uint32_t*>(overlapped);
  REQUIRE(xe::load_and_swap<uint32_t>(&words[6]) == X_ERROR_FILE_NOT_FOUND);
}

TEST_CASE("Kernel overlapped completion signals event", "[kernel]") {
  KernelStateFixture fixture;
  auto* memory = fixture.memory();
  auto* kernel_state = fixture.kernel_state();
  const uint32_t overlapped_ptr = AllocateOverlapped(memory);

  auto event = object_ref<XEvent>(new XEvent(kernel_state));
  event->Initialize(true, false);
  X_HANDLE event_handle = event->handle();
  REQUIRE(event_handle != 0);

  auto overlapped = memory->TranslateVirtual<XAM_OVERLAPPED*>(overlapped_ptr);
  overlapped->event = event_handle;

  uint32_t event_type = 0;
  uint32_t event_state = 0;
  event->Query(&event_type, &event_state);
  REQUIRE(event_state == 0);

  kernel_state->CompleteOverlappedEx(overlapped_ptr, X_ERROR_SUCCESS,
                                     X_ERROR_SUCCESS, 0);

  event->Query(&event_type, &event_state);
  REQUIRE(event_state != 0);

  REQUIRE(kernel_state->object_table()->ReleaseHandle(event_handle) ==
          X_STATUS_SUCCESS);
}

}  // namespace xe::kernel::test
