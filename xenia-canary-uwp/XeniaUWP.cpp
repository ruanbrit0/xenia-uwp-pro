#include "XeniaUWP.h"

#include "UWPUtil.h"
#include "WinRTKeyboard.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <system_error>
#include <thread>
#include <unordered_set>

#include "windowed_app_context_uwp.h"
#include "surface_uwp.h"
#include "window_uwp.h"

#include "third_party/imgui/imgui.h"

#include "xenia/emulator.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/memory.h"
#include "xenia/cpu/xex_module.h"
#include "xenia/ui/windowed_app.h"
#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/ui/window.h"
#include "xenia/ui/d3d12/d3d12_provider.h"
#include "xenia/gpu/d3d12/d3d12_graphics_system.h"
#include "xenia/hid/xinput/xinput_hid.h"
#include "xenia/hid/nop/nop_hid.h"
#include "xenia/apu/xaudio2/xaudio2_audio_system.h"
#include "xenia/config.h"
#include "xenia/base/main_win.h"
#include "xenia/vfs/devices/disc_image_device.h"
#include "xenia/vfs/devices/disc_zarchive_device.h"
#include "xenia/vfs/devices/xcontent_container_device.h"

using namespace xe;
using namespace xe::hid;

DECLARE_string(gamepaths);
DEFINE_string(gamepaths, "", "Paths the frontend will search for games.",
              "General");

static std::unique_ptr<ui::WindowedApp> app = nullptr;
static std::unique_ptr<ui::UWPWindowedAppContext> app_context = nullptr;
static ui::Window* s_window;
static Emulator* s_emulator;
static std::vector<std::string> s_paths;
static std::vector<std::tuple<std::string, std::string>> s_games;
static std::vector<std::string> s_scanned_paths;
static std::mutex s_game_list_mutex;
static std::atomic<uint64_t> s_scan_generation{0};
static std::atomic<bool> s_scanning_game_paths{false};
static std::atomic<uint64_t> s_scan_games_found{0};
static std::atomic<uint64_t> s_game_list_version{0};

void UWP::StartXenia() {
  app_context = std::make_unique<ui::UWPWindowedAppContext>();
  app = xe::ui::GetWindowedAppCreator()(*app_context.get());
  
  xe::InitializeWin32App(app->GetName());

  if (app->OnInitialize()) {
    RefreshPaths();
    // to-do, remodel this so it doesn't instantly shutdown.
    //app->InvokeOnDestroy();
  }

  //xe::ShutdownWin32App();
}

void UWP::ExecutePendingFunctionsFromUIThread() {
  app_context->ExecutePendingFunctionsFromUIThread();

  if (s_window) {
    app_context->CallInUIThread([=]() { s_window->RequestPaint(); });
  }
}

void UWP::RegisterXeniaWindow(xe::ui::Window* window) { s_window = window; }

void UWP::UpdateImGuiIO() {
  ImGuiIO& io = ImGui::GetIO();
  io.AddKeyEvent(ImGuiKey_Backspace, false);
  io.AddKeyEvent(ImGuiKey_Enter, false);

  {
    std::unique_lock lk(UWP::g_buffer_mutex);
    for (uint32_t c : UWP::g_char_buffer) {
      io.AddInputCharacter(c);

      if (c == '\b') {
        io.AddKeyEvent(ImGuiKey_Backspace, true);
      } else if (c == '\r') {
        io.AddKeyEvent(ImGuiKey_Enter, true);
      }
    }
    UWP::g_char_buffer.clear();
  }

  auto driver = static_cast<xe::ui::UWPWindow*>(s_window)->xinputdriver();
  if (!driver) return;

  hid::X_INPUT_STATE state;
  if (driver->GetState(0, &state) != X_STATUS_SUCCESS) return;

  io.AddKeyEvent(ImGuiKey_GamepadFaceDown, state.gamepad.buttons & X_INPUT_GAMEPAD_A);
  io.AddKeyEvent(ImGuiKey_GamepadFaceRight, state.gamepad.buttons & X_INPUT_GAMEPAD_B);
  io.AddKeyEvent(ImGuiKey_GamepadDpadLeft, state.gamepad.buttons & X_INPUT_GAMEPAD_DPAD_LEFT);
  io.AddKeyEvent(ImGuiKey_GamepadDpadRight, state.gamepad.buttons & X_INPUT_GAMEPAD_DPAD_RIGHT);
  io.AddKeyEvent(ImGuiKey_GamepadDpadUp, state.gamepad.buttons & X_INPUT_GAMEPAD_DPAD_UP);
  io.AddKeyEvent(ImGuiKey_GamepadDpadDown, state.gamepad.buttons & X_INPUT_GAMEPAD_DPAD_DOWN);
}

namespace {

using GameList = std::vector<std::tuple<std::string, std::string>>;

std::filesystem::path GetGameListCachePath() {
  return std::filesystem::path(UWP::GetLocalState()) / "game_index.tsv";
}

bool LoadGameListCache(const std::filesystem::path& cache_path,
                       GameList& games) {
  std::ifstream in(cache_path, std::ios::binary);
  if (!in) {
    return false;
  }

  GameList cached_games;
  std::string path;
  std::string name;
  while (in >> std::quoted(path) >> std::quoted(name)) {
    if (path.empty() || name.empty()) {
      continue;
    }
    cached_games.push_back({path, name});
  }

  if (cached_games.empty()) {
    return false;
  }

  games = std::move(cached_games);
  return true;
}

void SaveGameListCache(const std::filesystem::path& cache_path,
                       const GameList& games) {
  std::ofstream out(cache_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    XELOGE("UWP game scan failed to write cache: {}", cache_path.string());
    return;
  }

  for (const auto& game : games) {
    out << std::quoted(std::get<0>(game)) << '\t'
        << std::quoted(std::get<1>(game)) << '\n';
  }
}

void DeduplicateGameList(GameList& games) {
  std::unordered_set<std::string> seen_paths;
  GameList unique_games;
  seen_paths.reserve(games.size());
  unique_games.reserve(games.size());

  for (auto& game : games) {
    if (seen_paths.insert(std::get<0>(game)).second) {
      unique_games.push_back(std::move(game));
    }
  }

  games = std::move(unique_games);
}

std::string LowerExtension(const std::filesystem::path& path) {
  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return extension;
}

bool ShouldProbeGameFile(const std::filesystem::path& path) {
  if (_stricmp(path.filename().string().c_str(), "default.xex") == 0) {
    return true;
  }

  const std::string extension = LowerExtension(path);
  if (extension.empty()) {
    return true;
  }

  return extension == ".xex" || extension == ".iso" ||
         extension == ".xiso" || extension == ".zar" ||
         extension == ".con" || extension == ".live" ||
         extension == ".pirs";
}

xe::Emulator::FileSignatureType GetGameFileSignatureForScan(
    const std::filesystem::path& path) {
  if (!ShouldProbeGameFile(path)) {
    return xe::Emulator::FileSignatureType::Unknown;
  }

  const std::string extension = LowerExtension(path);
  if (extension == ".iso" || extension == ".xiso") {
    return xe::GetFileSignature(path);
  }

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return xe::Emulator::FileSignatureType::Unknown;
  }

  char file_magic[4] = {};
  if (!in.read(file_magic, sizeof(file_magic))) {
    return xe::Emulator::FileSignatureType::Unknown;
  }

  fourcc_t magic_value = make_fourcc(file_magic[0], file_magic[1],
                                     file_magic[2], file_magic[3]);
  switch (magic_value) {
    case xe::cpu::kXEX1Signature:
      return xe::Emulator::FileSignatureType::XEX1;
    case xe::cpu::kXEX2Signature:
      return xe::Emulator::FileSignatureType::XEX2;
    case xe::vfs::kCONSignature:
      return xe::Emulator::FileSignatureType::CON;
    case xe::vfs::kLIVESignature:
      return xe::Emulator::FileSignatureType::LIVE;
    case xe::vfs::kPIRSSignature:
      return xe::Emulator::FileSignatureType::PIRS;
    case xe::vfs::kXSFSignature:
      return xe::Emulator::FileSignatureType::XISO;
    default:
      break;
  }

  if (extension != ".zar" && !extension.empty()) {
    return xe::Emulator::FileSignatureType::Unknown;
  }

  in.clear();
  in.seekg(-static_cast<int>(sizeof(file_magic)), std::ios::end);
  if (!in.read(file_magic, sizeof(file_magic))) {
    return xe::Emulator::FileSignatureType::Unknown;
  }

  magic_value = make_fourcc(file_magic[0], file_magic[1], file_magic[2],
                            file_magic[3]);
  return magic_value == xe::vfs::kZarMagic
             ? xe::Emulator::FileSignatureType::ZAR
             : xe::Emulator::FileSignatureType::Unknown;
}

std::string GetLiveContainerName(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return path.stem().string();
  }

  in.seekg(0x412);

  char data[33] = {};
  for (int i = 0; i < 32; i++) {
    char utf16_char[2] = {};
    in.read(utf16_char, sizeof(utf16_char));
    char c = utf16_char[0];
    if (!in || c == 0) {
      break;
    }
    std::wctomb(&data[i], static_cast<wchar_t>(c));
  }

  return data[0] ? data : path.stem().string();
}

}  // namespace

void RecurseFolderForGames(
    std::string path, uint64_t generation,
    GameList& games) {
  std::error_code iterator_error;
  for (const auto& file :
       std::filesystem::directory_iterator(path, iterator_error)) {
    try {
      if (generation != s_scan_generation.load()) {
        return;
      }

      if (file.is_directory() && file.path().string() != path) {
        RecurseFolderForGames(file.path().string(), generation, games);
        continue;
      }

      if (!file.is_regular_file()) continue;

      switch (GetGameFileSignatureForScan(file.path())) {
        case xe::Emulator::FileSignatureType::XEX1:
        case xe::Emulator::FileSignatureType::XEX2: {
          std::string filename = "default";
          if (_stricmp(file.path().filename().string().c_str(),
                       "default.xex") == 0) {
            if (file.path().has_parent_path())
              filename = file.path().parent_path().filename().string();
          } else {
            filename = file.path().stem().string();
          }

          games.push_back({file.path().string(), filename});
          s_scan_games_found.store(games.size());
          break;
        }
        case xe::Emulator::FileSignatureType::CON:
        case xe::Emulator::FileSignatureType::PIRS:
        case xe::Emulator::FileSignatureType::ZAR: {
          std::string filename = file.path().stem().string();
          games.push_back({file.path().string(), filename});
          s_scan_games_found.store(games.size());
          break;
        }
        case xe::Emulator::FileSignatureType::LIVE: {
          games.push_back(
              {file.path().string(), GetLiveContainerName(file.path())});
          s_scan_games_found.store(games.size());
          break;
        }
        default:
          continue;
      }
    } catch (const std::exception&) {
      continue;
    }
  }

  if (iterator_error) {
    // This folder can't be opened.
    return;
  }
}

void UWP::RefreshPaths() {
  uint64_t generation = ++s_scan_generation;
  std::string local_cache = UWP::GetLocalCache();
  std::filesystem::path game_list_cache_path = GetGameListCachePath();
  std::vector<std::string> paths;

  if (!cvars::gamepaths.empty()) {
    std::stringstream ss(cvars::gamepaths);
    std::string item;
    while (std::getline(ss, item, ';')) {
      if (item.empty()) continue;
      paths.push_back(item);
    }
  }

  {
    std::lock_guard<std::mutex> lock(s_game_list_mutex);
    s_paths = paths;
  }

  GameList cached_games;
  if (LoadGameListCache(game_list_cache_path, cached_games)) {
    std::lock_guard<std::mutex> lock(s_game_list_mutex);
    if (s_games.empty()) {
      s_games = std::move(cached_games);
      s_game_list_version.fetch_add(1);
      XELOGI("UWP game list cache loaded: {} games", s_games.size());
    }
  }

  s_scanning_game_paths.store(true);
  s_scan_games_found.store(0);
  XELOGI("UWP RefreshPaths started: {} paths", paths.size());

  try {
    std::thread([generation, local_cache,
                 game_list_cache_path = std::move(game_list_cache_path),
                 paths = std::move(paths)]() {
      try {
        GameList games;
        XELOGI("UWP game scan begin");
        RecurseFolderForGames(local_cache, generation, games);

        for (const auto& path : paths) {
          if (generation != s_scan_generation.load()) {
            XELOGI("UWP game scan cancelled");
            return;
          }
          XELOGI("UWP game scan path begin: '{}'", path);
          RecurseFolderForGames(path, generation, games);
          XELOGI("UWP game scan path end: '{}'", path);
        }

        DeduplicateGameList(games);
        std::sort(games.begin(), games.end(), [](auto& first, auto& second) {
          return std::get<1>(first) < std::get<1>(second);
        });

        size_t game_count = 0;
        const GameList games_to_cache = games;
        {
          std::lock_guard<std::mutex> lock(s_game_list_mutex);
          if (generation != s_scan_generation.load()) {
            XELOGI("UWP game scan result discarded");
            return;
          }
          s_games = std::move(games);
          s_game_list_version.fetch_add(1);
          game_count = s_games.size();
        }

        if (generation == s_scan_generation.load()) {
          SaveGameListCache(game_list_cache_path, games_to_cache);
        }

        if (generation == s_scan_generation.load()) {
          s_scan_games_found.store(game_count);
          s_scanning_game_paths.store(false);
          XELOGI("UWP game scan end: {} games", game_count);
        }
      } catch (const std::exception& e) {
        if (generation == s_scan_generation.load()) {
          s_scanning_game_paths.store(false);
        }
        XELOGE("UWP game scan failed: {}", e.what());
      } catch (...) {
        if (generation == s_scan_generation.load()) {
          s_scanning_game_paths.store(false);
        }
        XELOGE("UWP game scan failed with unknown exception");
      }
    }).detach();
  } catch (const std::exception& e) {
    s_scanning_game_paths.store(false);
    XELOGE("UWP failed to start game scan: {}", e.what());
  }
}

std::vector<std::tuple<std::string, std::string>> UWP::GetGames() {
  std::lock_guard<std::mutex> lock(s_game_list_mutex);
  return s_games;
}

uint64_t UWP::GetGameListVersion() { return s_game_list_version.load(); }

void UWP::SetGamePaths(std::vector<std::string> paths) {
  XELOGI("UWP SetGamePaths begin: {} paths", paths.size());
  std::stringstream ss;
  for (auto p : paths) {
    ss << p << ";";
  }

  {
    std::lock_guard<std::mutex> lock(s_game_list_mutex);
    s_paths = paths;
  }

  auto cpaths = dynamic_cast<cvar::ConfigVar<std::string>*>(
      cvar::ConfigVars->find("gamepaths")->second);
  cpaths->SetConfigValue(ss.str());
  config::SaveConfig();
  XELOGI("UWP SetGamePaths end");
  RefreshPaths();
}

std::vector<std::string> UWP::GetPaths() {
  std::lock_guard<std::mutex> lock(s_game_list_mutex);
  return s_paths;
}

bool UWP::IsScanningGamePaths() { return s_scanning_game_paths.load(); }

uint64_t UWP::GetGameScanFoundCount() { return s_scan_games_found.load(); }

std::string UWP::GetGameScanStatus() {
  if (!s_scanning_game_paths.load()) {
    return "Idle";
  }

  std::stringstream status;
  status << "Scanning game paths: " << s_scan_games_found.load()
         << " games found";
  return status.str();
}
