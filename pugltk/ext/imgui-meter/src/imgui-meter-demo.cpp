#include "imgui-meter/imgui-meter.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace ImGuiMeter {

constexpr char kFormat[] = "%.2f";

struct ImGuiMeterDemoWindowData {
  float level_min = -60.0f;
  float level_max = 6.0f;
  float mono_level = 0.0f;
  float stereo_levels[2] = {0.0f, 0.0f};
};

void ShowDemoWindow(bool* p_open) {
  // Stored data
  static ImGuiMeterDemoWindowData demo_data;

  if (ImGui::Begin("imgui-meter-demo", p_open)) {
    ImGui::SliderFloat("mono", &demo_data.mono_level, demo_data.level_min, demo_data.level_max, kFormat, ImGuiSliderFlags_AlwaysClamp);

    ImGuiMeter::AudioLevelMeter("mono-level", &demo_data.mono_level, 1, demo_data.level_min, demo_data.level_max);


    ImGui::SliderFloat2("stereo", demo_data.stereo_levels, demo_data.level_min, demo_data.level_max, kFormat, ImGuiSliderFlags_AlwaysClamp);

    ImGuiMeter::AudioLevelMeter("stereo-levels", demo_data.stereo_levels, 2, demo_data.level_min, demo_data.level_max);

    ImGui::End();
  }
}

}  // namespace ImGuiMeter
