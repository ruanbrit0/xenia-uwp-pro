/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>

#include "third_party/catch/include/catch.hpp"
#include "third_party/fmt/include/fmt/format.h"
#include "xenia/kernel/testing/kernel_test_util.h"
#include "xenia/kernel/util/shim_utils.h"
#include "xenia/kernel/xam/content_manager.h"
#include "xenia/kernel/xam/xam_content_device.h"

namespace xe::kernel::xam {

dword_result_t XamContentCreate_entry(dword_t user_index, lpstring_t root_name,
                                      lpvoid_t content_data_ptr, dword_t flags,
                                      lpdword_t disposition_ptr,
                                      lpdword_t license_mask_ptr,
                                      lpvoid_t overlapped_ptr);

dword_result_t XamContentResolve_entry(dword_t user_index,
                                       lpvoid_t content_data_ptr,
                                       lpunknown_t buffer_ptr,
                                       dword_t buffer_size, dword_t unk1,
                                       dword_t unk2, dword_t unk3);

dword_result_t XamContentGetDeviceVolumePath_entry(dword_t device_id,
                                                   lpunknown_t path_ptr,
                                                   dword_t path_size,
                                                   dword_t append_backslash);

dword_result_t XamContentCreateEnumerator_entry(
    dword_t user_index, dword_t device_id, dword_t content_type,
    dword_t content_flags, dword_t items_per_enumerate,
    lpdword_t buffer_size_ptr, lpdword_t handle_out);

dword_result_t XamContentAggregateCreateEnumerator_entry(
    qword_t xuid, dword_t device_id, dword_t content_type, unknown_t unk3,
    lpdword_t handle_out);

dword_result_t XamContentOpenFile_entry(
    dword_t user_index, lpstring_t root_name, lpstring_t path, dword_t flags,
    lpdword_t disposition_ptr, lpdword_t license_mask_ptr,
    lpvoid_t overlapped_ptr);

dword_result_t XamContentClose_entry(lpstring_t root_name,
                                     lpunknown_t overlapped_ptr);

dword_result_t XamContentGetCreator_entry(dword_t user_index,
                                          lpvoid_t content_data_ptr,
                                          lpdword_t is_creator_ptr,
                                          lpqword_t creator_xuid_ptr,
                                          lpunknown_t overlapped_ptr);

dword_result_t XamContentGetThumbnail_entry(dword_t user_index,
                                            lpvoid_t content_data_ptr,
                                            lpvoid_t buffer_ptr,
                                            lpdword_t buffer_size_ptr,
                                            lpunknown_t overlapped_ptr);

dword_result_t XamContentSetThumbnail_entry(dword_t user_index,
                                            lpvoid_t content_data_ptr,
                                            lpvoid_t buffer_ptr,
                                            dword_t buffer_size,
                                            lpunknown_t overlapped_ptr);

dword_result_t XamContentDelete_entry(dword_t user_index,
                                      lpvoid_t content_data_ptr,
                                      lpunknown_t overlapped_ptr);

dword_result_t XamEnumerate_entry(dword_t handle, dword_t flags,
                                  lpvoid_t buffer, dword_t buffer_length,
                                  lpdword_t items_returned,
                                  pointer_t<XAM_OVERLAPPED> overlapped);

}  // namespace xe::kernel::xam

namespace xe::kernel::test {

namespace {

constexpr uint32_t kCreateNew = 1;
constexpr uint32_t kOpenExisting = 3;
constexpr uint32_t kOpenAlways = 4;

class ScopedContentRoot {
 public:
  ScopedContentRoot() {
    auto ticks = std::chrono::high_resolution_clock::now()
                     .time_since_epoch()
                     .count();
    path_ = std::filesystem::temp_directory_path() /
            fmt::format("xenia-kernel-content-test-{}", ticks);
    REQUIRE(std::filesystem::create_directories(path_));
  }

  ScopedContentRoot(const ScopedContentRoot&) = delete;
  ScopedContentRoot& operator=(const ScopedContentRoot&) = delete;

  ~ScopedContentRoot() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

xam::XCONTENT_AGGREGATE_DATA MakeSavedGameContent(
    const std::string_view file_name) {
  xam::XCONTENT_AGGREGATE_DATA data;
  std::memset(&data, 0, sizeof(data));
  data.device_id = static_cast<uint32_t>(xam::DummyDeviceId::HDD);
  data.content_type = XContentType::kSavedGame;
  data.title_id = 0x12345678;
  data.unk134 = 1;
  data.set_display_name(u"Test Save");
  data.set_file_name(file_name);
  return data;
}

xam::XCONTENT_DATA MakeSavedGameContentData(
    const std::string_view file_name) {
  xam::XCONTENT_DATA data;
  std::memset(&data, 0, sizeof(data));
  data.device_id = static_cast<uint32_t>(xam::DummyDeviceId::HDD);
  data.content_type = XContentType::kSavedGame;
  data.set_display_name(u"Test Save");
  data.set_file_name(file_name);
  return data;
}

xam::XCONTENT_AGGREGATE_DATA MakeExpectedExportContent(
    const xam::XCONTENT_DATA& content_data) {
  xam::XCONTENT_AGGREGATE_DATA aggregate_data(content_data);
  return aggregate_data;
}

uint32_t LoadGuestDword(const uint32_t* value) {
  return xe::load_and_swap<uint32_t>(value);
}

uint64_t LoadGuestQword(const uint64_t* value) {
  return xe::load_and_swap<uint64_t>(value);
}

void StoreGuestDword(uint32_t* value, uint32_t new_value) {
  xe::store_and_swap<uint32_t>(value, new_value);
}

void CreateExportContent(char* root_name, xam::XCONTENT_DATA* content_data) {
  uint32_t disposition = 0;
  uint32_t license_mask = 0;
  REQUIRE(uint32_t(xam::XamContentCreate_entry(
              0, root_name, content_data, kCreateNew, &disposition,
              &license_mask, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&disposition) == 1);
}

}  // namespace

TEST_CASE("ContentManager creates opens and closes content", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* content_manager = fixture.kernel_state()->content_manager();
  auto content_data = MakeSavedGameContent("SAVE0001");

  REQUIRE_FALSE(content_manager->ContentExists(content_data));

  REQUIRE(content_manager->CreateContent("save", content_data) ==
          X_ERROR_SUCCESS);
  REQUIRE(content_manager->ContentExists(content_data));
  REQUIRE(content_manager->IsContentOpen(content_data));

  REQUIRE(content_manager->CreateContent("save", content_data) ==
          X_ERROR_ALREADY_EXISTS);

  REQUIRE(content_manager->CloseContent("save") == X_ERROR_SUCCESS);
  REQUIRE_FALSE(content_manager->IsContentOpen(content_data));

  REQUIRE(content_manager->OpenContent("save", content_data) ==
          X_ERROR_SUCCESS);
  REQUIRE(content_manager->IsContentOpen(content_data));

  REQUIRE(content_manager->CloseContent("save") == X_ERROR_SUCCESS);
}

TEST_CASE("ContentManager delete respects open content", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* content_manager = fixture.kernel_state()->content_manager();
  auto content_data = MakeSavedGameContent("SAVE0002");

  REQUIRE(content_manager->DeleteContent(content_data) ==
          X_ERROR_FILE_NOT_FOUND);
  REQUIRE(content_manager->CreateContent("save", content_data) ==
          X_ERROR_SUCCESS);

  REQUIRE(content_manager->DeleteContent(content_data) ==
          X_ERROR_ACCESS_DENIED);
  REQUIRE(content_manager->CloseContent("save") == X_ERROR_SUCCESS);

  REQUIRE(content_manager->DeleteContent(content_data) == X_ERROR_SUCCESS);
  REQUIRE_FALSE(content_manager->ContentExists(content_data));
}

TEST_CASE("ContentManager lists content without header", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* content_manager = fixture.kernel_state()->content_manager();
  auto content_data = MakeSavedGameContent("SAVE0003");

  REQUIRE(content_manager->CreateContent("save", content_data) ==
          X_ERROR_SUCCESS);
  REQUIRE(content_manager->CloseContent("save") == X_ERROR_SUCCESS);

  auto entries = content_manager->ListContent(
      static_cast<uint32_t>(xam::DummyDeviceId::HDD), XContentType::kSavedGame,
      0x12345678);

  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].device_id == content_data.device_id);
  REQUIRE(entries[0].content_type == content_data.content_type);
  REQUIRE(entries[0].title_id == content_data.title_id);
  REQUIRE(entries[0].file_name() == content_data.file_name());
}

TEST_CASE("XamContentCreate creates content synchronously", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* kernel_state = fixture.kernel_state();
  auto* content_manager = kernel_state->content_manager();
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("EXPORT001");
  auto expected_content = MakeExpectedExportContent(content_data);
  uint32_t disposition = 0;
  uint32_t license_mask = 0xFFFFFFFF;

  auto result = xam::XamContentCreate_entry(
      0, root_name, &content_data, kCreateNew, &disposition, &license_mask,
      static_cast<void*>(nullptr));

  REQUIRE(uint32_t(result) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&disposition) == 1);
  REQUIRE(LoadGuestDword(&license_mask) == 0);
  REQUIRE(content_manager->ContentExists(expected_content));
  REQUIRE(content_manager->IsContentOpen(expected_content));

  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE_FALSE(content_manager->IsContentOpen(expected_content));
}

TEST_CASE("XamContentCreate opens existing content synchronously",
          "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* kernel_state = fixture.kernel_state();
  auto* content_manager = kernel_state->content_manager();
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("EXPORT002");
  auto expected_content = MakeExpectedExportContent(content_data);
  uint32_t disposition = 0;
  uint32_t license_mask = 0;

  REQUIRE(uint32_t(xam::XamContentCreate_entry(
              0, root_name, &content_data, kCreateNew, &disposition,
              &license_mask, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&disposition) == 1);
  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);

  disposition = 0;
  license_mask = 0xFFFFFFFF;
  REQUIRE(uint32_t(xam::XamContentCreate_entry(
              0, root_name, &content_data, kOpenExisting, &disposition,
              &license_mask, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&disposition) == 2);
  REQUIRE(LoadGuestDword(&license_mask) == 0);
  REQUIRE(content_manager->IsContentOpen(expected_content));

  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
}

TEST_CASE("XamContentCreate open existing fails for missing content",
          "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("MISSING1");
  uint32_t disposition = 0xFFFFFFFF;
  uint32_t license_mask = 0xFFFFFFFF;

  auto result = xam::XamContentCreate_entry(
      0, root_name, &content_data, kOpenExisting, &disposition, &license_mask,
      static_cast<void*>(nullptr));

  REQUIRE(uint32_t(result) == X_ERROR_PATH_NOT_FOUND);
  REQUIRE(LoadGuestDword(&disposition) == 0);
  REQUIRE(LoadGuestDword(&license_mask) == 0xFFFFFFFF);
}

TEST_CASE("XamContentOpenFile creates saved game content", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* kernel_state = fixture.kernel_state();
  auto* content_manager = kernel_state->content_manager();
  char root_name[] = "save";
  char path[] = "EXPORT003";
  auto expected_content = MakeSavedGameContent(path);
  expected_content.title_id = kernel_state->title_id();
  expected_content.unk134 = kernel_state->user_profile(uint32_t(0))->xuid();
  uint32_t disposition = 0;
  uint32_t license_mask = 0xFFFFFFFF;

  auto result = xam::XamContentOpenFile_entry(
      0, root_name, path, kOpenAlways, &disposition, &license_mask,
      static_cast<void*>(nullptr));

  REQUIRE(uint32_t(result) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&disposition) == 1);
  REQUIRE(LoadGuestDword(&license_mask) == 0);
  REQUIRE(content_manager->ContentExists(expected_content));
  REQUIRE(content_manager->IsContentOpen(expected_content));

  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
}

TEST_CASE("XamContentResolve returns HDD content path", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto content_data = MakeSavedGameContentData("RESOLVE001");
  char resolved_path[128] = {};

  auto result = xam::XamContentResolve_entry(
      0, &content_data, resolved_path, sizeof(resolved_path), 0, 0, 0);

  REQUIRE(uint32_t(result) == X_ERROR_SUCCESS);
  REQUIRE(std::string(resolved_path) ==
          "\\Device\\Harddisk0\\Partition1\\Content\\00000000\\"
          "00000001\\RESOLVE001");
}

TEST_CASE("XamContentGetDeviceVolumePath returns HDD volume", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  char volume_path[16] = {};

  uint32_t result = uint32_t(xam::XamContentGetDeviceVolumePath_entry(
      static_cast<uint32_t>(xam::DummyDeviceId::HDD), volume_path,
      sizeof(volume_path), 1));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(std::string(volume_path) == "hdd0\\");

  std::memset(volume_path, 0, sizeof(volume_path));
  result = uint32_t(xam::XamContentGetDeviceVolumePath_entry(
      static_cast<uint32_t>(xam::DummyDeviceId::ODD), volume_path,
      sizeof(volume_path), 1));
  REQUIRE(result == X_ERROR_FUNCTION_FAILED);
}

TEST_CASE("XamContentCreateEnumerator enumerates saved games", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("ENUM001");
  uint32_t buffer_size = 0;
  uint32_t handle = 0;
  uint32_t items_returned = 0;
  xam::XCONTENT_DATA items[2] = {};

  CreateExportContent(root_name, &content_data);
  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);

  uint32_t result = uint32_t(xam::XamContentCreateEnumerator_entry(
      0, static_cast<uint32_t>(xam::DummyDeviceId::HDD),
      static_cast<uint32_t>(XContentType::kSavedGame), 0, 2, &buffer_size,
      &handle));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&buffer_size) == sizeof(items));
  REQUIRE(LoadGuestDword(&handle) != 0);

  result = uint32_t(xam::XamEnumerate_entry(
      LoadGuestDword(&handle), 0, items, sizeof(items), &items_returned,
      static_cast<XAM_OVERLAPPED*>(nullptr)));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&items_returned) == 1);
  REQUIRE(items[0].device_id == content_data.device_id);
  REQUIRE(items[0].content_type == content_data.content_type);
  REQUIRE(items[0].file_name() == content_data.file_name());

  REQUIRE(fixture.kernel_state()->object_table()->ReleaseHandle(
              LoadGuestDword(&handle)) == X_STATUS_SUCCESS);
}

TEST_CASE("XamEnumerate reports no more content items", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("ENUM002");
  uint32_t buffer_size = 0;
  uint32_t handle = 0;
  uint32_t items_returned = 0;
  xam::XCONTENT_DATA item = {};

  CreateExportContent(root_name, &content_data);
  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(uint32_t(xam::XamContentCreateEnumerator_entry(
              0, static_cast<uint32_t>(xam::DummyDeviceId::HDD),
              static_cast<uint32_t>(XContentType::kSavedGame), 0, 1,
              &buffer_size, &handle)) == X_ERROR_SUCCESS);

  uint32_t result = uint32_t(xam::XamEnumerate_entry(
      LoadGuestDword(&handle), 0, &item, sizeof(item), &items_returned,
      static_cast<XAM_OVERLAPPED*>(nullptr)));
  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&items_returned) == 1);

  result = uint32_t(xam::XamEnumerate_entry(
      LoadGuestDword(&handle), 0, &item, sizeof(item), &items_returned,
      static_cast<XAM_OVERLAPPED*>(nullptr)));
  REQUIRE(result == X_ERROR_NO_MORE_FILES);
  REQUIRE(LoadGuestDword(&items_returned) == 0);

  REQUIRE(fixture.kernel_state()->object_table()->ReleaseHandle(
              LoadGuestDword(&handle)) == X_STATUS_SUCCESS);
}

TEST_CASE("XamContentAggregateCreateEnumerator enumerates HDD saves",
          "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("AGGREG01");
  uint32_t handle = 0;
  uint32_t items_returned = 0;
  xam::XCONTENT_AGGREGATE_DATA items[1] = {};

  CreateExportContent(root_name, &content_data);
  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);

  uint32_t result = uint32_t(xam::XamContentAggregateCreateEnumerator_entry(
      0, static_cast<uint32_t>(xam::DummyDeviceId::HDD),
      static_cast<uint32_t>(XContentType::kSavedGame), 0, &handle));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&handle) != 0);

  result = uint32_t(xam::XamEnumerate_entry(
      LoadGuestDword(&handle), 0, items, sizeof(items), &items_returned,
      static_cast<XAM_OVERLAPPED*>(nullptr)));

  REQUIRE(result == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&items_returned) == 1);
  REQUIRE(items[0].device_id == content_data.device_id);
  REQUIRE(items[0].content_type == content_data.content_type);
  REQUIRE(items[0].title_id == fixture.kernel_state()->title_id());
  REQUIRE(items[0].file_name() == content_data.file_name());

  REQUIRE(fixture.kernel_state()->object_table()->ReleaseHandle(
              LoadGuestDword(&handle)) == X_STATUS_SUCCESS);
}

TEST_CASE("XamContentAggregateCreateEnumerator rejects invalid devices",
          "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  uint32_t handle = 0;

  auto result = xam::XamContentAggregateCreateEnumerator_entry(
      0, 0xDEADBEEF, static_cast<uint32_t>(XContentType::kSavedGame), 0,
      &handle);

  REQUIRE(uint32_t(result) == X_E_INVALIDARG);
  REQUIRE(LoadGuestDword(&handle) == 0);
}

TEST_CASE("XamContentDelete respects open content", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* kernel_state = fixture.kernel_state();
  auto* content_manager = kernel_state->content_manager();
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("EXPORT004");
  auto expected_content = MakeExpectedExportContent(content_data);

  CreateExportContent(root_name, &content_data);
  REQUIRE(content_manager->ContentExists(expected_content));

  REQUIRE(uint32_t(xam::XamContentDelete_entry(
              0, &content_data, static_cast<void*>(nullptr))) ==
          X_ERROR_ACCESS_DENIED);

  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(uint32_t(xam::XamContentDelete_entry(
              0, &content_data, static_cast<void*>(nullptr))) ==
          X_ERROR_SUCCESS);
  REQUIRE_FALSE(content_manager->ContentExists(expected_content));
}

TEST_CASE("XamContentGetCreator reports signed in save owner", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  auto* kernel_state = fixture.kernel_state();
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("EXPORT005");
  uint32_t is_creator = 0;
  uint64_t creator_xuid = 0;

  CreateExportContent(root_name, &content_data);

  REQUIRE(uint32_t(xam::XamContentGetCreator_entry(
              0, &content_data, &is_creator, &creator_xuid,
              static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&is_creator) == 1);
  REQUIRE(LoadGuestQword(&creator_xuid) ==
          kernel_state->user_profile(uint32_t(0))->xuid());

  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
}

TEST_CASE("XamContent thumbnail roundtrips through exports", "[kernel]") {
  ScopedContentRoot content_root;
  KernelStateFixture fixture(content_root.path());
  char root_name[] = "save";
  auto content_data = MakeSavedGameContentData("EXPORT006");
  uint8_t thumbnail[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A};
  uint8_t thumbnail_read[sizeof(thumbnail)] = {};
  uint32_t thumbnail_read_size = 0;

  CreateExportContent(root_name, &content_data);

  REQUIRE(uint32_t(xam::XamContentSetThumbnail_entry(
              0, &content_data, thumbnail, sizeof(thumbnail),
              static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);

  StoreGuestDword(&thumbnail_read_size, sizeof(thumbnail_read));
  REQUIRE(uint32_t(xam::XamContentGetThumbnail_entry(
              0, &content_data, thumbnail_read, &thumbnail_read_size,
              static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
  REQUIRE(LoadGuestDword(&thumbnail_read_size) == sizeof(thumbnail));
  REQUIRE(std::memcmp(thumbnail_read, thumbnail, sizeof(thumbnail)) == 0);

  REQUIRE(uint32_t(xam::XamContentClose_entry(
              root_name, static_cast<void*>(nullptr))) == X_ERROR_SUCCESS);
}

}  // namespace xe::kernel::test
