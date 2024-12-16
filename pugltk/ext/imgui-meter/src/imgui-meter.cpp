#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui-meter/imgui-meter.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdlib>
#include <vector>

namespace ImGuiMeter {

template <typename DataType>
bool BaseAudioLevelMeter(const char *label, ImGuiDataType data_type, DataType const *data, size_t data_size,
                         DataType v_min, DataType v_max, ImGuiMeterVariant variant, ImVec2 size, ImGuiMeterFlags flags,
                         DataType v_clip, ImColor const &color_clip_highlight) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const float w = ImGui::CalcItemWidth();

  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
  const ImRect frame_bb(window->DC.CursorPos,
                        window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
  const ImRect total_bb(
      frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, id, &frame_bb, 0)) return false;

  const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);

  // Draw frame
  const ImU32 frame_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive
                                             : hovered        ? ImGuiCol_FrameBgHovered
                                                              : ImGuiCol_FrameBg);

  ImGui::RenderNavCursor(frame_bb, id);
  ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

  const float level_padding = 2.0f;  // FIXME: Should be part of style.
  ImRect levels_bb;
  levels_bb.Min = frame_bb.Min + ImVec2(level_padding, level_padding);
  levels_bb.Max = frame_bb.Max - ImVec2(level_padding, level_padding);

  float single_level_delta;
  ImRect single_level_bb;
  if (variant == ImGuiMeterVariant_HorizontalBars) {
    single_level_delta = (levels_bb.GetSize().y - (data_size - 1) * level_padding) / data_size;
    single_level_bb.Min = ImVec2{levels_bb.Min.x, levels_bb.Min.y};                        // Min=Upper-left
    single_level_bb.Max = ImVec2{levels_bb.Max.x, levels_bb.Min.y + single_level_delta};  // Max=Lower-right
  } else {
    single_level_delta = (levels_bb.GetSize().x - (data_size - 1) * level_padding) / data_size;
    single_level_bb.Min = ImVec2{levels_bb.Min.x, levels_bb.Min.y};                        // Min=Upper-left
    single_level_bb.Max = ImVec2{levels_bb.Min.x + single_level_delta, levels_bb.Max.y };  // Max=Lower-right
  }

  const ImU32 level_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
  const auto level_clip = (static_cast<float>(v_clip) - v_min) / (v_max - v_min);

  // Render levels
  for (size_t i = 0; i < data_size; ++i) {
    // Render single level
    const auto level = (static_cast<float>(data[i]) - v_min) / (v_max - v_min);
    ImRect level_bb;

    if (variant == ImGuiMeterVariant_HorizontalBars) {
      level_bb.Min = ImVec2{single_level_bb.Min.x, single_level_bb.Min.y};  // Min=Upper-left
      level_bb.Max = ImVec2{single_level_bb.Min.x + level * single_level_bb.GetSize().x,
                            single_level_bb.Max.y};  // Max=Lower-right
    } else {
      level_bb.Min = ImVec2{single_level_bb.Min.x,
                            single_level_bb.Min.y + (1.0f - level) * single_level_bb.GetSize().y};  // Min=Upper-left
      level_bb.Max = ImVec2{single_level_bb.Max.x, single_level_bb.Max.y};                          // Max=Lower-right
    }
    window->DrawList->AddRectFilled(level_bb.Min, level_bb.Max, level_col, style.GrabRounding);

    // Render clipping highlight
    if (data[i] > v_clip && !(flags & ImGuiMeterFlags_NoClipHighlight)) {
      ImRect clip_bb;
      if (variant == ImGuiMeterVariant_HorizontalBars) {
        clip_bb.Min = ImVec2{single_level_bb.Min.x + level_clip * single_level_bb.GetSize().x,
                             single_level_bb.Min.y};  // Min=Upper-left
        clip_bb.Max = ImVec2{single_level_bb.Min.x + level * single_level_bb.GetSize().x,
                             single_level_bb.Max.y};  // Max=Lower-right
      } else {
        clip_bb.Min = ImVec2{single_level_bb.Min.x,
                             single_level_bb.Min.y + (1.0f - level) * single_level_bb.GetSize().y};  // Min=Upper-left
        clip_bb.Max =
            ImVec2{single_level_bb.Max.x,
                   single_level_bb.Min.y + (1.0f - level_clip) * single_level_bb.GetSize().y};  // Min=Upper-left
      }
      window->DrawList->AddRectFilled(clip_bb.Min, clip_bb.Max, color_clip_highlight, style.GrabRounding);
    }

    // calc next level bounding box
    if (variant == ImGuiMeterVariant_HorizontalBars) {
      single_level_bb.Min.y = single_level_bb.Max.y + level_padding;        // Min=Upper-left
      single_level_bb.Max.y = single_level_bb.Min.y + single_level_delta;  // Max=Lower-right
    } else {
      single_level_bb.Min.x = single_level_bb.Max.x + level_padding;        // Min=Upper-left
      single_level_bb.Max.x = single_level_bb.Min.x + single_level_delta;  // Max=Lower-right
    }
  }

  // Render label
  if (label_size.x > 0.0f)
    ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

  return false;
}

bool AudioLevelMeter(const char *label, float const *data, size_t data_size, float v_min, float v_max,
                     ImGuiMeterVariant variant, ImVec2 size, ImGuiMeterFlags flags, float v_clip,
                     ImColor const &color_clip_highlight) {
  return BaseAudioLevelMeter(label, ImGuiDataType_Float, data, data_size, v_min, v_max, variant, size, flags, v_clip,
                             color_clip_highlight);
}

}  // namespace ImGuiMeter
