/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2023 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/vfs/device.h"
#include "xenia/vfs/devices/stfs_xbox.h"
#include "xenia/vfs/devices/xcontent_container_entry.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <future>
#include <tuple>
#include <vector>

#include "third_party/catch/include/catch.hpp"
#include "third_party/fmt/include/fmt/format.h"

namespace xe::vfs {

struct XContentContainerEntryTestPeer {
  static void SetBlocks(
      XContentContainerEntry* entry, size_t size,
      std::vector<XContentContainerEntry::BlockRecord> block_list) {
    entry->size_ = size;
    entry->allocation_size_ = size;
    entry->data_size_ = size;
    entry->block_list_ = std::move(block_list);
  }
};

namespace test {

namespace {

class DummyDevice final : public Device {
 public:
  DummyDevice() : Device("test:") {}

  bool Initialize() override { return true; }
  void Dump(StringBuffer* string_buffer) override {}
  Entry* ResolvePath(const std::string_view path) override { return nullptr; }
  const std::string& name() const override { return name_; }
  uint32_t attributes() const override { return 0; }
  uint32_t component_name_max_length() const override { return 255; }
  uint32_t total_allocation_units() const override { return 0; }
  uint32_t available_allocation_units() const override { return 0; }
  uint32_t sectors_per_allocation_unit() const override { return 8; }
  uint32_t bytes_per_sector() const override { return 0x200; }

 private:
  std::string name_ = "Test XContent";
};

class ScopedTestFile {
 public:
  explicit ScopedTestFile(const std::vector<uint8_t>& data) {
    auto ticks =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() /
            fmt::format("xenia-vfs-test-{}.bin", ticks);

    FILE* write_file = nullptr;
    REQUIRE(fopen_s(&write_file, path_.string().c_str(), "wb") == 0);
    REQUIRE(write_file != nullptr);
    REQUIRE(fwrite(data.data(), 1, data.size(), write_file) == data.size());
    REQUIRE(fclose(write_file) == 0);

    REQUIRE(fopen_s(&file_, path_.string().c_str(), "rb") == 0);
    REQUIRE(file_ != nullptr);
  }

  ScopedTestFile(const ScopedTestFile&) = delete;
  ScopedTestFile& operator=(const ScopedTestFile&) = delete;

  ~ScopedTestFile() {
    if (file_) {
      fclose(file_);
    }
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }

  FILE* file() const { return file_; }

 private:
  std::filesystem::path path_;
  FILE* file_ = nullptr;
};

class ScopedXContentFile {
 public:
  ScopedXContentFile(
      FILE* host_file, size_t virtual_size,
      std::vector<XContentContainerEntry::BlockRecord> block_list)
      : ScopedXContentFile(std::vector<FILE*>{host_file}, virtual_size,
                           std::move(block_list)) {}

  ScopedXContentFile(
      const std::vector<FILE*>& host_files, size_t virtual_size,
      std::vector<XContentContainerEntry::BlockRecord> block_list)
      : entry_(&device_, nullptr, "test.bin", &files_) {
    for (size_t i = 0; i < host_files.size(); ++i) {
      files_.emplace(std::piecewise_construct, std::forward_as_tuple(i),
                     std::forward_as_tuple(host_files[i]));
    }
    XContentContainerEntryTestPeer::SetBlocks(&entry_, virtual_size,
                                              std::move(block_list));
    REQUIRE(entry_.Open(FileAccess::kFileReadData, &file_) == X_STATUS_SUCCESS);
    REQUIRE(file_ != nullptr);
  }

  ScopedXContentFile(const ScopedXContentFile&) = delete;
  ScopedXContentFile& operator=(const ScopedXContentFile&) = delete;

  ~ScopedXContentFile() {
    if (file_) {
      file_->Destroy();
    }
  }

  File* file() const { return file_; }

 private:
  MultiFileHandles files_;
  DummyDevice device_;
  XContentContainerEntry entry_;
  File* file_ = nullptr;
};

std::vector<uint8_t> MakePattern(size_t size, uint8_t seed) {
  std::vector<uint8_t> data(size);
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = static_cast<uint8_t>((i * 17 + seed) & 0xFF);
  }
  return data;
}

}  // namespace

TEST_CASE("STFS Decode date and time", "[stfs_decode]") {
  SECTION("10 June 2022 19:46:00 UTC - Decode") {
    const uint16_t date = 0x54CA;
    const uint16_t time = 0x9DBD;
    const uint64_t result = 132993639580000000;

    const uint64_t timestamp = decode_fat_timestamp(date, time);

    REQUIRE(timestamp == result);
  }
}

TEST_CASE("XContent fragmented file reads", "[xcontent]") {
  auto host_data = MakePattern(0x4000, 0x31);
  ScopedTestFile host_file(host_data);
  ScopedXContentFile xcontent_file(
      host_file.file(), 0x1800,
      {{0, 0x1000, 0x800}, {0, 0x2000, 0x400}, {0, 0x3000, 0xC00}});

  SECTION("read across block records") {
    std::vector<uint8_t> actual(0x900);
    size_t bytes_read = 0;

    REQUIRE(xcontent_file.file()->ReadSync(actual.data(), actual.size(), 0x700,
                                           &bytes_read) == X_STATUS_SUCCESS);

    std::vector<uint8_t> expected;
    expected.insert(expected.end(), host_data.begin() + 0x1700,
                    host_data.begin() + 0x1800);
    expected.insert(expected.end(), host_data.begin() + 0x2000,
                    host_data.begin() + 0x2400);
    expected.insert(expected.end(), host_data.begin() + 0x3000,
                    host_data.begin() + 0x3400);
    REQUIRE(bytes_read == expected.size());
    REQUIRE(actual == expected);
  }

  SECTION("read trims at virtual EOF") {
    std::vector<uint8_t> actual(0x400, 0xCD);
    size_t bytes_read = 0;

    REQUIRE(xcontent_file.file()->ReadSync(actual.data(), actual.size(), 0x1700,
                                           &bytes_read) == X_STATUS_SUCCESS);

    std::vector<uint8_t> expected(host_data.begin() + 0x3B00,
                                  host_data.begin() + 0x3C00);
    REQUIRE(bytes_read == expected.size());
    REQUIRE(std::vector<uint8_t>(actual.begin(), actual.begin() + bytes_read) ==
            expected);
    REQUIRE(actual[bytes_read] == 0xCD);
  }

  SECTION("zero length read succeeds") {
    uint8_t actual = 0xCD;
    size_t bytes_read = 123;

    REQUIRE(xcontent_file.file()->ReadSync(&actual, 0, 0x100, &bytes_read) ==
            X_STATUS_SUCCESS);
    REQUIRE(bytes_read == 0);
    REQUIRE(actual == 0xCD);
  }

  SECTION("read starting at EOF fails") {
    uint8_t actual = 0;
    size_t bytes_read = 123;

    REQUIRE(xcontent_file.file()->ReadSync(&actual, sizeof(actual), 0x1800,
                                           &bytes_read) ==
            X_STATUS_END_OF_FILE);
  }
}

TEST_CASE("XContent reads across multiple host files", "[xcontent]") {
  auto first_host_data = MakePattern(0x3000, 0x13);
  auto second_host_data = MakePattern(0x3000, 0x57);
  ScopedTestFile first_host_file(first_host_data);
  ScopedTestFile second_host_file(second_host_data);
  ScopedXContentFile xcontent_file(
      {first_host_file.file(), second_host_file.file()}, 0x1800,
      {{0, 0x0800, 0x800}, {1, 0x1000, 0x800}, {0, 0x2000, 0x800}});

  std::vector<uint8_t> actual(0x1200);
  size_t bytes_read = 0;

  REQUIRE(xcontent_file.file()->ReadSync(actual.data(), actual.size(), 0x400,
                                         &bytes_read) == X_STATUS_SUCCESS);

  std::vector<uint8_t> expected;
  expected.insert(expected.end(), first_host_data.begin() + 0x0C00,
                  first_host_data.begin() + 0x1000);
  expected.insert(expected.end(), second_host_data.begin() + 0x1000,
                  second_host_data.begin() + 0x1800);
  expected.insert(expected.end(), first_host_data.begin() + 0x2000,
                  first_host_data.begin() + 0x2600);
  REQUIRE(bytes_read == expected.size());
  REQUIRE(actual == expected);
}

TEST_CASE("XContent concurrent reads share host file safely", "[xcontent]") {
  auto host_data = MakePattern(0x10000, 0x7B);
  ScopedTestFile host_file(host_data);
  ScopedXContentFile xcontent_file(host_file.file(), 0x8000,
                                   {{0, 0x0000, 0x1000},
                                    {0, 0x3000, 0x1000},
                                    {0, 0x7000, 0x1000},
                                    {0, 0xB000, 0x1000},
                                    {0, 0xC000, 0x1000},
                                    {0, 0xD000, 0x1000},
                                    {0, 0xE000, 0x1000},
                                    {0, 0xF000, 0x1000}});

  std::vector<uint8_t> expected;
  expected.insert(expected.end(), host_data.begin() + 0x0000,
                  host_data.begin() + 0x1000);
  expected.insert(expected.end(), host_data.begin() + 0x3000,
                  host_data.begin() + 0x4000);
  expected.insert(expected.end(), host_data.begin() + 0x7000,
                  host_data.begin() + 0x8000);
  expected.insert(expected.end(), host_data.begin() + 0xB000,
                  host_data.begin() + 0x10000);

  auto read_task = [&]() {
    for (size_t i = 0; i < 250; ++i) {
      std::vector<uint8_t> actual(expected.size());
      size_t bytes_read = 0;
      if (xcontent_file.file()->ReadSync(actual.data(), actual.size(), 0,
                                         &bytes_read) != X_STATUS_SUCCESS) {
        return false;
      }
      if (bytes_read != expected.size() || actual != expected) {
        return false;
      }
    }
    return true;
  };

  std::vector<std::future<bool>> tasks;
  for (size_t i = 0; i < 8; ++i) {
    tasks.push_back(std::async(std::launch::async, read_task));
  }
  for (auto& task : tasks) {
    REQUIRE(task.get());
  }
}

}  // namespace test
}  // namespace xe::vfs
