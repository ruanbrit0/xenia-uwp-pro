#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace winrt {
struct fire_and_forget;
}

namespace xe {
namespace ui {
class Window;
}
}  // namespace xe

namespace UWP {
void StartXenia();
void ExecutePendingFunctionsFromUIThread();
void RegisterXeniaWindow(xe::ui::Window* window);
void UpdateImGuiIO();

void RefreshPaths();
std::vector<std::tuple<std::string, std::string>> GetGames();
std::vector<std::string> GetPaths();
void SetGamePaths(std::vector<std::string> paths);
bool IsScanningGamePaths();
uint64_t GetGameScanFoundCount();
std::string GetGameScanStatus();
}
