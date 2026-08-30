/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_TESTING_KERNEL_TEST_UTIL_H_
#define XENIA_KERNEL_TESTING_KERNEL_TEST_UTIL_H_

#include <filesystem>
#include <memory>

#include "third_party/catch/include/catch.hpp"
#include "xenia/cpu/backend/x64/x64_backend.h"
#include "xenia/cpu/export_resolver.h"
#include "xenia/cpu/processor.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/memory.h"
#include "xenia/vfs/virtual_file_system.h"

namespace xe::kernel::test {

class KernelStateFixture {
 public:
  explicit KernelStateFixture(const std::filesystem::path& content_root = {}) {
    memory_ = std::make_unique<Memory>();
    REQUIRE(memory_->Initialize());

    export_resolver_ = std::make_unique<cpu::ExportResolver>();
    processor_ = std::make_unique<cpu::Processor>(memory_.get(),
                                                  export_resolver_.get());

    std::unique_ptr<cpu::backend::Backend> backend;
#if XE_ARCH_AMD64
    backend = std::make_unique<cpu::backend::x64::X64Backend>();
#endif  // XE_ARCH_AMD64
    REQUIRE(backend != nullptr);
    REQUIRE(processor_->Setup(std::move(backend)));

    file_system_ = std::make_unique<vfs::VirtualFileSystem>();
    kernel_state_ = KernelState::CreateForTesting(
        memory_.get(), processor_.get(), file_system_.get(), content_root);
  }

  ~KernelStateFixture() {
    for (auto& object : kernel_state_->object_table()->GetAllObjects()) {
      for (auto handle : object->handles()) {
        kernel_state_->object_table()->ReleaseHandle(handle);
      }
    }
    kernel_state_.reset();
  }

  Memory* memory() const { return memory_.get(); }
  KernelState* kernel_state() const { return kernel_state_.get(); }
  vfs::VirtualFileSystem* file_system() const { return file_system_.get(); }

 private:
  std::unique_ptr<Memory> memory_;
  std::unique_ptr<cpu::ExportResolver> export_resolver_;
  std::unique_ptr<cpu::Processor> processor_;
  std::unique_ptr<vfs::VirtualFileSystem> file_system_;
  std::unique_ptr<KernelState> kernel_state_;
};

}  // namespace xe::kernel::test

#endif  // XENIA_KERNEL_TESTING_KERNEL_TEST_UTIL_H_
