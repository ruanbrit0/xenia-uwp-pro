#include "UWPUtil.h"

#include "xenia/base/logging.h"
#include "xenia/emulator.h"

#include <agents.h>
#include <ppl.h>
#include <ppltasks.h>
#include <fstream>
#include <iostream>
#include <string>

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/windows.graphics.display.core.h>

using namespace winrt::Windows::Storage::Pickers;
namespace UWP {
std::string m_game_path = "";
int m_DPI = 96;
bool m_ui_open = false;

winrt::fire_and_forget PickGame(xe::Emulator* emu) {
  FileOpenPicker openPicker;
  openPicker.ViewMode(PickerViewMode::List);
  openPicker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
  openPicker.FileTypeFilter().Append(L"*");

  auto file = co_await openPicker.PickSingleFileAsync();
  if (file) {
    std::string path = winrt::to_string(file.Path().data());

    try {
      XELOGI("UWP launching picked title: {}", path);
      auto result = emu->LaunchPath(path);
      XELOGI("UWP picked title launch result: {:08X}", result);
    } catch (const std::exception& e) {
      XELOGE("UWP picked title launch failed with exception: {}", e.what());
    } catch (...) {
      XELOGE("UWP picked title launch failed with unknown exception");
    }
  }
}

winrt::fire_and_forget PickFolderAsync(
    std::function<void(std::string)> callback) {
  FolderPicker openPicker;
  openPicker.ViewMode(PickerViewMode::List);
  openPicker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
  openPicker.FileTypeFilter().Append(L"*");

  auto folder = co_await openPicker.PickSingleFolderAsync();
  std::string path = "";
  if (folder) {
    path = winrt::to_string(folder.Path().data());
  }

  callback(path);
}

winrt::fire_and_forget PickFilesAsync(
    std::function<void(std::vector<std::string>)> callback) {
  FileOpenPicker openPicker;
  openPicker.ViewMode(PickerViewMode::List);
  openPicker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
  openPicker.FileTypeFilter().Append(L"*");

  auto folders = co_await openPicker.PickMultipleFilesAsync();
  std::vector<std::string> paths;
  if (folders) {
    for (auto folder : folders) {
      paths.push_back(winrt::to_string(folder.Path()));
    }
  }

  callback(paths);
}

winrt::fire_and_forget PickFileAsync(
    std::function<void(std::string)> callback) {
  FileOpenPicker openPicker;
  openPicker.ViewMode(PickerViewMode::List);
  openPicker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
  openPicker.FileTypeFilter().Append(L"*");

  auto folder = co_await openPicker.PickSingleFileAsync();
  std::string path = "";
  if (folder) {
    path = winrt::to_string(folder.Path().data());
  }

  callback(path);
}

bool HasGamePath() {
  bool has_game_path = !m_game_path.empty();
  if (has_game_path) {
    XELOGI("UWP HasGamePath: true, path='{}'", m_game_path);
  }
  return has_game_path;
}

std::string ConsumeGamePath() {
  XELOGI("UWP ConsumeGamePath begin: path='{}'", m_game_path);
  std::string game_path = m_game_path;
  m_game_path.clear();
  XELOGI("UWP ConsumeGamePath end: path='{}'", game_path);
  return game_path;
}

void SelectGameFromWinRT(xe::Emulator* emu) {
  XELOGI("UWP SelectGameFromWinRT begin: has_path={}", !m_game_path.empty());
  if (m_game_path == "") {
    XELOGI("UWP SelectGameFromWinRT opening picker");
    PickGame(emu);
  } else {
    try {
      XELOGI("UWP launching title from automatic path: {}", m_game_path);
      auto result = emu->LaunchPath(m_game_path);
      XELOGI("UWP automatic launch result: {:08X}", result);
    } catch (const std::exception& e) {
      XELOGE("UWP automatic launch failed with exception: {}", e.what());
    } catch (...) {
      XELOGE("UWP automatic launch failed with unknown exception");
    }
    m_game_path.clear();
  }
}

void SelectFolder(std::function<void(std::string)> callback) {
  PickFolderAsync(callback);
}

void SelectFile(std::function<void(std::string)> callback) {
  PickFileAsync(callback);
}

void SelectFiles(std::function<void(std::vector<std::string>)> callback) {
  PickFilesAsync(callback);
}

bool TestPathPermissions(std::string path) {
  auto p = path + "\\text.txt";
  std::ofstream o(p);
  bool success = o.good();
  std::remove(p.c_str());

  return success;
}

std::string GetLocalCache() {
  return winrt::to_string(winrt::Windows::Storage::ApplicationData::Current()
                              .LocalCacheFolder()
                              .Path());
}

std::string GetLocalState() {
  return winrt::to_string(
      winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path());
}

std::string GetInstalledLocation() {
  return winrt::to_string(winrt::Windows::ApplicationModel::Package::Current()
                              .InstalledLocation()
                              .Path());
}

int GetCoreDPI() { return m_DPI; }

void SetAutomaticLaunch(std::string game_path) {
  XELOGI("UWP SetAutomaticLaunch: path='{}'", game_path);
  m_game_path = game_path;
}
void SetDPI(int DPI) { m_DPI = DPI; }
bool IsUIOpen() { return m_ui_open; }
void SetUIOpen(bool is_open) { m_ui_open = is_open; }
}  // namespace UWP
