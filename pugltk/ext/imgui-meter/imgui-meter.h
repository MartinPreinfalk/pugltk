#pragma once

#include <imgui.h>

#include <cstdlib>

typedef int ImGuiMeterFlags;

enum ImGuiMeterFlags_ { ImGuiMeterFlags_NoTitle = 1 << 0 };

typedef int ImGuiMeterVariant;

enum ImGuiMeterVariant_ { ImGuiMeterVariant_Bars = 1 << 0 };

namespace ImGuiMeter {

bool AudioLevelMeter(const char *label, float const *values_db, size_t n_values, float v_min_db = -60.0f,
                     float v_max_db = 6.0f, const char *format = "%.1f",
                     ImGuiMeterVariant variant = ImGuiMeterVariant_Bars, ImVec2 size = ImVec2{0, 0},
                     ImGuiMeterFlags flags = 0);

}  // namespace ImGuiMeter
