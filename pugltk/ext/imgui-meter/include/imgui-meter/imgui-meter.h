#pragma once

#include <imgui.h>

#include <array>
#include <cstdlib>

typedef int ImGuiMeterFlags;

enum ImGuiMeterFlags_ {
  ImGuiMeterFlags_Vertical = 1 << 0,
  ImGuiMeterFlags_NoTitle = 1 << 1,
  ImGuiMeterFlags_Linear = 1 << 2,
  ImGuiMeterFlags_FixSizeVerticalBars = 1 << 3
};

namespace ImGuiMeter {

template <typename DataType>
struct ColorMapEntry {
  DataType lo = 0.0f, hi = 0.0f;
  ImColor color = ImColor(0, 0, 0, 0);
};

template <typename DataType>
using MapFunction = float (*)(DataType const &level, DataType const &v_min, DataType const &v_max);

bool AudioLevelMeter(const char *label, float const *data, size_t data_size, ImGuiMeterFlags flags = 0,
                     float v_min = -60.0f, float v_max = 6.0f, ImVec2 size = ImVec2{0, 0},
                     ColorMapEntry<float> const *color_map = nullptr, size_t color_map_size = 0,
                     MapFunction<float> map_func = nullptr);

bool AudioLevelMeter(const char *label, double const *data, size_t data_size, ImGuiMeterFlags flags = 0,
                     double v_min = -60.0, double v_max = 6.0, ImVec2 size = ImVec2{0, 0},
                     ColorMapEntry<double> const *color_map = nullptr, size_t color_map_size = 0,
                     MapFunction<double> map_func = nullptr);

void ShowDemoWindow(bool *p_open = nullptr);  // create Demo window.

}  // namespace ImGuiMeter
