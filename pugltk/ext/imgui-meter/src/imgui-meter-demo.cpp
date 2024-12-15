#include <imgui.h>
#include <imgui_internal.h>

#include "imgui-meter/imgui-meter.h"

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
    if (ImGui::CollapsingHeader("Mono")) {
      ImGui::SliderFloat("mono", &demo_data.mono_level, demo_data.level_min, demo_data.level_max, kFormat,
                         ImGuiSliderFlags_AlwaysClamp);

      ImGuiMeter::AudioLevelMeter("mono-level##0", &demo_data.mono_level, 1);
      ImGuiMeter::AudioLevelMeter("mono-level##1", &demo_data.mono_level, 1, ImGuiMeterFlags_Linear);
      ImGuiMeter::AudioLevelMeter("mono-level##2", &demo_data.mono_level, 1, ImGuiMeterFlags_NoTitle);
      ImGuiMeter::AudioLevelMeter("mono-level##3", &demo_data.mono_level, 1, ImGuiMeterFlags_Vertical);
      ImGuiMeter::AudioLevelMeter("mono-level##4", &demo_data.mono_level, 1,
                                  ImGuiMeterFlags_Vertical | ImGuiMeterFlags_NoTitle);
      ImGuiMeter::AudioLevelMeter("mono-level##5", &demo_data.mono_level, 1,
                                  ImGuiMeterFlags_Vertical | ImGuiMeterFlags_FixSizeVerticalBars);
    }

    if (ImGui::CollapsingHeader("Stereo")) {
      ImGui::SliderFloat2("stereo", demo_data.stereo_levels, demo_data.level_min, demo_data.level_max, kFormat,
                          ImGuiSliderFlags_AlwaysClamp);

      ImGuiMeter::AudioLevelMeter("stereo-levels##0", demo_data.stereo_levels, 2);
      ImGuiMeter::AudioLevelMeter("stereo-levels##1", demo_data.stereo_levels, 2, ImGuiMeterFlags_Linear);
      ImGuiMeter::AudioLevelMeter("stereo-levels##2", demo_data.stereo_levels, 2, ImGuiMeterFlags_NoTitle);
      ImGuiMeter::AudioLevelMeter("stereo-levels##3", demo_data.stereo_levels, 2, ImGuiMeterFlags_Vertical);
      ImGuiMeter::AudioLevelMeter("stereo-levels##4", demo_data.stereo_levels, 2,
                                  ImGuiMeterFlags_Vertical | ImGuiMeterFlags_NoTitle);
      ImGuiMeter::AudioLevelMeter("stereo-levels##5", demo_data.stereo_levels, 2,
                                  ImGuiMeterFlags_Vertical | ImGuiMeterFlags_FixSizeVerticalBars);
    }
    ImGui::End();
  }
}

}  // namespace ImGuiMeter
