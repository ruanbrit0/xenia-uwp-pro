/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <cstring>
#include <string>

#include "third_party/catch/include/catch.hpp"
#include "xenia/kernel/testing/kernel_test_util.h"
#include "xenia/kernel/util/shim_utils.h"
#include "xenia/kernel/xam/user_profile.h"
#include "xenia/kernel/xam/xam_content_device.h"
#include "xenia/xbox.h"

namespace xe::kernel::xam {

dword_result_t XamContentGetLicenseMask_entry(lpdword_t mask_ptr,
                                              lpunknown_t overlapped_ptr);

dword_result_t XamContentGetDeviceName_entry(dword_t device_id,
                                             lpu16string_t name_buffer,
                                             dword_t name_capacity);

dword_result_t XamContentGetDeviceState_entry(dword_t device_id,
                                              lpunknown_t overlapped_ptr);

dword_result_t XamContentGetDeviceData_entry(
    dword_t device_id, pointer_t<X_CONTENT_DEVICE_DATA> device_data);

dword_result_t XamContentCreateDeviceEnumerator_entry(
    dword_t content_type, dword_t content_flags, dword_t max_count,
    lpdword_t buffer_size_ptr, lpdword_t handle_out);

dword_result_t XamEnumerate_entry(dword_t handle, dword_t flags,
                                  lpvoid_t buffer, dword_t buffer_length,
                                  lpdword_t items_returned,
                                  pointer_t<XAM_OVERLAPPED> overlapped);

X_HRESULT_result_t XamUserGetXUID_entry(dword_t user_index, dword_t type_mask,
                                        lpqword_t xuid_ptr);

dword_result_t XamUserGetIndexFromXUID_entry(qword_t xuid, dword_t flags,
                                             lpdword_t index);

dword_result_t XamUserGetSigninState_entry(dword_t user_index);

X_HRESULT_result_t XamUserGetSigninInfo_entry(
    dword_t user_index, dword_t flags, pointer_t<X_USER_SIGNIN_INFO> info);

dword_result_t XamUserGetName_entry(dword_t user_index, lpstring_t buffer,
                                    dword_t buffer_len);

dword_result_t XamUserGetGamerTag_entry(dword_t user_index,
                                        lpu16string_t buffer,
                                        dword_t buffer_len);

dword_result_t XamUserLogon_entry(lpqword_t xuids_ptr, dword_t flags,
                                  pointer_t<XAM_OVERLAPPED> overlapped_ptr);

}  // namespace xe::kernel::xam

namespace xe::kernel::test {

namespace {

uint32_t LoadGuestDword(const uint32_t* value) {
  return xe::load_and_swap<uint32_t>(value);
}

uint64_t LoadGuestQword(const uint64_t* value) {
  return xe::load_and_swap<uint64_t>(value);
}

std::u16string LoadGuestU16String(const char16_t* value) {
  return xe::load_and_swap<std::u16string>(value);
}

}  // namespace

TEST_CASE("XamContentGetLicenseMask returns configured mask", "[kernel]") {
  KernelStateFixture fixture;
  uint32_t license_mask = 0;

  auto result = xam::XamContentGetLicenseMask_entry(
      &license_mask, static_cast<void*>(nullptr));

  REQUIRE(uint32_t(result) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&license_mask) == 1);
}

TEST_CASE("XamContentGetDeviceName returns dummy device names", "[kernel]") {
  KernelStateFixture fixture;
  char16_t name[28] = {};

  uint32_t result = uint32_t(xam::XamContentGetDeviceName_entry(
      static_cast<uint32_t>(xam::DummyDeviceId::HDD), name,
      uint32_t(xe::countof(name))));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestU16String(name) == u"Dummy HDD");

  result = uint32_t(xam::XamContentGetDeviceName_entry(
      static_cast<uint32_t>(xam::DummyDeviceId::HDD), name, 4));
  REQUIRE(result == X_ERROR_INSUFFICIENT_BUFFER);
}

TEST_CASE("XamContentGetDeviceState reports dummy device presence",
          "[kernel]") {
  KernelStateFixture fixture;

  REQUIRE(uint32_t(xam::XamContentGetDeviceState_entry(
              static_cast<uint32_t>(xam::DummyDeviceId::HDD),
              static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(uint32_t(xam::XamContentGetDeviceState_entry(
              0xDEADBEEF, static_cast<void*>(nullptr))) ==
          X_ERROR_DEVICE_NOT_CONNECTED);
}

TEST_CASE("XamContentGetDeviceData returns dummy HDD data", "[kernel]") {
  KernelStateFixture fixture;
  xam::X_CONTENT_DEVICE_DATA device_data;

  auto result = xam::XamContentGetDeviceData_entry(
      static_cast<uint32_t>(xam::DummyDeviceId::HDD), &device_data);

  REQUIRE(uint32_t(result) == X_ERROR_SUCCESS);
  REQUIRE(device_data.device_id ==
          static_cast<uint32_t>(xam::DummyDeviceId::HDD));
  REQUIRE(device_data.device_type ==
          static_cast<uint32_t>(xam::DeviceType::HDD));
  REQUIRE(device_data.total_bytes == 20ull * 1024ull * 1024ull * 1024ull);
  REQUIRE(device_data.free_bytes == 3ull * 1024ull * 1024ull * 1024ull);
  REQUIRE(LoadGuestU16String(device_data.name_chars) == u"Dummy HDD");
}

TEST_CASE("XamContentCreateDeviceEnumerator enumerates dummy devices",
          "[kernel]") {
  KernelStateFixture fixture;
  uint32_t buffer_size = 0;
  uint32_t handle = 0;
  uint32_t items_returned = 0;
  xam::X_CONTENT_DEVICE_DATA items[2] = {};

  uint32_t result = uint32_t(xam::XamContentCreateDeviceEnumerator_entry(
      static_cast<uint32_t>(XContentType::kSavedGame), 0,
      uint32_t(xe::countof(items)), &buffer_size, &handle));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&buffer_size) == sizeof(items));
  REQUIRE(LoadGuestDword(&handle) != 0);

  result = uint32_t(xam::XamEnumerate_entry(
      LoadGuestDword(&handle), 0, items, sizeof(items), &items_returned,
      static_cast<XAM_OVERLAPPED*>(nullptr)));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&items_returned) == 2);
  REQUIRE(items[0].device_id ==
          static_cast<uint32_t>(xam::DummyDeviceId::HDD));
  REQUIRE(items[1].device_id ==
          static_cast<uint32_t>(xam::DummyDeviceId::ODD));

  REQUIRE(fixture.kernel_state()->object_table()->ReleaseHandle(
              LoadGuestDword(&handle)) == X_STATUS_SUCCESS);
}

TEST_CASE("XamEnumerate rejects invalid handles", "[kernel]") {
  KernelStateFixture fixture;
  uint32_t items_returned = 0xFFFFFFFF;
  xam::X_CONTENT_DEVICE_DATA item = {};

  auto result = xam::XamEnumerate_entry(
      0xDEADBEEF, 0, &item, sizeof(item), &items_returned,
      static_cast<XAM_OVERLAPPED*>(nullptr));

  REQUIRE(uint32_t(result) == X_ERROR_INVALID_HANDLE);
  REQUIRE(LoadGuestDword(&items_returned) == 0);
}

TEST_CASE("XamUser basic profile exports return default user", "[kernel]") {
  KernelStateFixture fixture;
  auto* kernel_state = fixture.kernel_state();
  uint64_t xuid = 0;
  char name[16] = {};

  REQUIRE(uint32_t(xam::XamUserGetSigninState_entry(0)) ==
          kernel_state->user_profile(uint32_t(0))->signin_state());
  REQUIRE(uint32_t(xam::XamUserGetSigninState_entry(0xFF)) ==
          kernel_state->user_profile(uint32_t(0))->signin_state());
  REQUIRE(uint32_t(xam::XamUserGetSigninState_entry(4)) == 0);

  REQUIRE(uint32_t(xam::XamUserGetXUID_entry(0, 3, &xuid)) == X_E_SUCCESS);
  REQUIRE(LoadGuestQword(&xuid) ==
          kernel_state->user_profile(uint32_t(0))->xuid());

  REQUIRE(uint32_t(xam::XamUserGetName_entry(0, name, sizeof(name))) ==
          X_E_SUCCESS);
  REQUIRE(std::string(name) ==
          kernel_state->user_profile(uint32_t(0))->name());
}

TEST_CASE("XamUser profile lookup exports use default user", "[kernel]") {
  KernelStateFixture fixture;
  auto* kernel_state = fixture.kernel_state();
  const uint64_t user_xuid = kernel_state->user_profile(uint32_t(0))->xuid();
  uint32_t index = 0xFFFFFFFF;
  xam::X_USER_SIGNIN_INFO signin_info = {};
  char16_t gamer_tag[16] = {};

  REQUIRE(uint32_t(xam::XamUserGetIndexFromXUID_entry(user_xuid, 0, &index)) ==
          X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&index) == 0);

  REQUIRE(uint32_t(xam::XamUserGetIndexFromXUID_entry(
              user_xuid + 1, 0, &index)) == X_E_NO_SUCH_USER);

  REQUIRE(uint32_t(xam::XamUserGetSigninInfo_entry(0, 0, &signin_info)) ==
          X_E_SUCCESS);
  REQUIRE(signin_info.xuid == user_xuid);
  REQUIRE(signin_info.signin_state ==
          kernel_state->user_profile(uint32_t(0))->signin_state());
  REQUIRE(std::string(signin_info.name) ==
          kernel_state->user_profile(uint32_t(0))->name());

  REQUIRE(uint32_t(xam::XamUserGetGamerTag_entry(
              0, gamer_tag, uint32_t(xe::countof(gamer_tag)))) == X_E_SUCCESS);
  REQUIRE(LoadGuestU16String(gamer_tag) ==
          xe::to_utf16(kernel_state->user_profile(uint32_t(0))->name()));
}

TEST_CASE("XamUserLogon fills default XUID synchronously", "[kernel]") {
  KernelStateFixture fixture;
  auto* kernel_state = fixture.kernel_state();
  uint64_t xuids[4] = {};

  auto result = xam::XamUserLogon_entry(xuids, 0,
                                        static_cast<XAM_OVERLAPPED*>(nullptr));

  REQUIRE(uint32_t(result) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestQword(&xuids[0]) ==
          kernel_state->user_profile(uint32_t(0))->xuid());
}

}  // namespace xe::kernel::test
