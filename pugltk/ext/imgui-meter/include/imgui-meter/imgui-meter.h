#pragma once

#include <imgui.h>

#include <cstdlib>

typedef int ImGuiMeterFlags;

enum ImGuiMeterFlags_ { ImGuiMeterFlags_NoTitle = 1 << 0, ImGuiMeterFlags_NoClipHighlight = 1 << 1 };

typedef int ImGuiMeterVariant;

enum ImGuiMeterVariant_ { ImGuiMeterVariant_HorizontalBars = 1 << 0, ImGuiMeterVariant_VerticalBars = 1 << 1 };

namespace ImGuiMeter {

bool AudioLevelMeter(const char *label, float const *data, size_t data_size, float v_min = -60.0f, float v_max = 6.0f,
                     ImGuiMeterVariant variant = ImGuiMeterVariant_HorizontalBars, ImVec2 size = ImVec2{0, 0},
                     ImGuiMeterFlags flags = 0, float v_clip = 0.0f,
                     ImColor const &color_clip_highlight = ImColor(255, 0, 0, 255));

void ShowDemoWindow(bool *p_open = NULL);  // create Demo window.

}  // namespace ImGuiMeter
