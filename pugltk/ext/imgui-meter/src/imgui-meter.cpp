#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui-meter/imgui-meter.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdlib>
#include <vector>

namespace {
enum class Orientation { Horizontal, Vertical };

constexpr std::array<ImGuiMeter::ColorMapEntry<float>, 5> kDefaultColorMapFloat = {
    ImGuiMeter::ColorMapEntry<float>{-60.0f, -18.0f, ImColor(0.0f, 0.5f, 0.0f)},
    ImGuiMeter::ColorMapEntry<float>{-18.0f, -9.0f, ImColor(0.0f, 1.0f, 0.0f)},
    ImGuiMeter::ColorMapEntry<float>{-9.0f, -3.0f, ImColor(1.0f, 1.0f, 0.0f)},
    ImGuiMeter::ColorMapEntry<float>{-3.0f, 0.0f, ImColor(1.0f, 0.5f, 0.0f)},
    ImGuiMeter::ColorMapEntry<float>{0.0f, 6.0f, ImColor(1.0f, 0.0f, 0.0f)}};

constexpr std::array<ImGuiMeter::ColorMapEntry<double>, 5> kDefaultColorMapDouble = {
    ImGuiMeter::ColorMapEntry<double>{-60.0, -18.0, ImColor(0.0f, 0.5f, 0.0f)},
    ImGuiMeter::ColorMapEntry<double>{-18.0, -9.0, ImColor(0.0f, 1.0f, 0.0f)},
    ImGuiMeter::ColorMapEntry<double>{-9.0, -3.0, ImColor(1.0f, 1.0f, 0.0f)},
    ImGuiMeter::ColorMapEntry<double>{-3.0, 0.0, ImColor(1.0f, 0.5f, 0.0f)},
    ImGuiMeter::ColorMapEntry<double>{0.0, 6.0, ImColor(1.0f, 0.0f, 0.0f)}};

template <typename DataType>
float DefaultMap(DataType const &level, DataType const &v_min, DataType const &v_max) {
  constexpr DataType k = 0.05;
  const DataType term_min = ::atan(k * v_min);
  const DataType term_max = ::atan(k * v_max);
  auto v = (::atan(k * level) - term_min) / (term_max - term_min);
  v = std::min<DataType>(1.0, std::max<DataType>(0.0, v));
  return v;
}

template <typename DataType>
float LinearMap(DataType const &level, DataType const &v_min, DataType const &v_max) {
  auto v = (level - v_min) / (v_max - v_min);
  v = std::min<DataType>(1.0, std::max<DataType>(0.0, v));
  return v;
}

}  // namespace

namespace ImGuiMeter {

template <typename DataType>
void DrawBarPart(Orientation const &orientation, DataType const &lo, DataType const &hi,
                 ColorMapEntry<DataType> const &color_map_entry, ImRect const &bb, MapFunction<DataType> map_func,
                 DataType const &v_min, DataType const &v_max) {
  if (hi <= lo) return;  // early exit
  auto lo_norm = map_func(lo, v_min, v_max);
  auto hi_norm = map_func(hi, v_min, v_max);
  ImRect part;
  switch (orientation) {
    case Orientation::Horizontal:
      part.Min.x = bb.Min.x + lo_norm * bb.GetSize().x;
      part.Min.y = bb.Min.y;
      part.Max.x = bb.Min.x + hi_norm * bb.GetSize().x;
      part.Max.y = bb.Max.y;
      break;
    case Orientation::Vertical:
      part.Min.x = bb.Min.x;
      part.Min.y = bb.Max.y - hi_norm * bb.GetSize().y;
      part.Max.x = bb.Max.x;
      part.Max.y = bb.Max.y - lo_norm * bb.GetSize().y;
      break;
  }
  ImGui::GetCurrentWindow()->DrawList->AddRectFilled(part.Min, part.Max, color_map_entry.color,
                                                     ImGui::GetStyle().GrabRounding);
}

template <typename DataType>
void DrawBar(Orientation const &orientation, DataType const &level, DataType const &v_min, DataType const &v_max,
             ImRect const &bb, ColorMapEntry<DataType> const *color_map, size_t color_map_size,
             MapFunction<DataType> map_func) {
  for (size_t i = 0; i < color_map_size; ++i) {
    auto lo = (i == 0) ? v_min : color_map[i].lo;
    if (level > color_map[i].hi) {
      DrawBarPart(orientation, lo, color_map[i].hi, color_map[i], bb, map_func, v_min, v_max);
    } else {
      DrawBarPart(orientation, lo, level, color_map[i], bb, map_func, v_min, v_max);
      break;  // rest of ranges must not be drawn
    }
  }
}

template <typename DataType>
bool BaseAudioLevelMeter(const char *label, ImGuiDataType data_type, DataType const *data, size_t data_size,
                         ImGuiMeterFlags flags, DataType v_min, DataType v_max, ImVec2 size,
                         ColorMapEntry<DataType> const *color_map, size_t color_map_size,
                         MapFunction<DataType> map_func) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;
  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = (flags & ImGuiMeterFlags_NoTitle) ? ImVec2{0, 0} : ImGui::CalcTextSize(label, NULL, true);
  const Orientation orientation = (flags & ImGuiMeterFlags_Vertical) ? Orientation::Vertical : Orientation::Horizontal;

  if (map_func == nullptr) {
    if (flags & ImGuiMeterFlags_Linear) {
      map_func = &LinearMap<DataType>;
    } else {
      map_func = &DefaultMap<DataType>;
    }
  }

  // calculate default size
  if (size == ImVec2(0, 0)) {
    switch (orientation) {
      case Orientation::Horizontal:
        if (flags & ImGuiMeterFlags_NoTitle) {
          size.x = ImGui::CalcItemWidth();
          size.y = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;
        } else {
          size.x = ImGui::CalcItemWidth() + style.ItemInnerSpacing.x + label_size.x;
          size.y = label_size.y + style.FramePadding.y * 2.0f;
        }
        break;
      case Orientation::Vertical:
        if (flags & ImGuiMeterFlags_NoTitle) {
          size.x = ImGui::GetTextLineHeight() + style.ItemInnerSpacing.x * 2.0f;
          size.y = ImGui::GetTextLineHeight() * 5.0f + style.FramePadding.y * 2.0f;
        } else {
          size.x = std::max(ImGui::GetTextLineHeight(), label_size.x) + style.ItemInnerSpacing.x * 2.0f;
          size.y =
              label_size.y + ImGui::GetTextLineHeight() * 5.0f + style.FramePadding.y * 2.0f + style.ItemInnerSpacing.y;
        }
        break;
    }
  }

  // calculate bounding boxes from size
  const auto total_bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + size);
  ImRect frame_bb;
  if (flags & ImGuiMeterFlags_NoTitle) {
    frame_bb = total_bb;
  } else {
    switch (orientation) {
      case Orientation::Horizontal:
        frame_bb =
            ImRect(total_bb.Min, ImVec2(total_bb.Max.x - label_size.x - style.ItemInnerSpacing.x, total_bb.Max.y));
        break;
      case Orientation::Vertical:
        frame_bb =
            ImRect(total_bb.Min, ImVec2(total_bb.Max.x, total_bb.Max.y - label_size.y - style.ItemInnerSpacing.y));
        break;
    }
  }

  // Setup item
  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, id, &frame_bb, 0)) return false;
  const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);

  // Draw frame
  const ImU32 frame_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive
                                             : hovered        ? ImGuiCol_FrameBgHovered
                                                              : ImGuiCol_FrameBg);

  ImGui::RenderNavCursor(frame_bb, id);
  ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

  // calulate levels bounding box
  const float bar_padding = 4.0f;  // FIXME: Should be part of style.
  ImRect all_bars_bb;
  if (orientation == Orientation::Vertical && flags & ImGuiMeterFlags_FixSizeVerticalBars) {
    all_bars_bb.Min.x = frame_bb.GetCenter().x - 0.5 * ImGui::GetTextLineHeight();
    all_bars_bb.Min.y = frame_bb.Min.y + bar_padding;
    all_bars_bb.Max.x = frame_bb.GetCenter().x + 0.5 * ImGui::GetTextLineHeight();
    all_bars_bb.Max.y = frame_bb.Max.y - bar_padding;
  } else {
    all_bars_bb.Min = frame_bb.Min + ImVec2(bar_padding, bar_padding);
    all_bars_bb.Max = frame_bb.Max - ImVec2(bar_padding, bar_padding);
  }

  // calculate single level bounding box (same as levels bounding box for mono)
  float bar_delta;
  ImRect bar_bb;
  switch (orientation) {
    case Orientation::Horizontal:
      bar_delta = (all_bars_bb.GetSize().y - (data_size - 1) * bar_padding) / data_size;
      bar_bb.Min = ImVec2{all_bars_bb.Min.x, all_bars_bb.Min.y};              // Min=Upper-left
      bar_bb.Max = ImVec2{all_bars_bb.Max.x, all_bars_bb.Min.y + bar_delta};  // Max=Lower-right
      break;
    case Orientation::Vertical:
      bar_delta = (all_bars_bb.GetSize().x - (data_size - 1) * bar_padding) / data_size;
      bar_bb.Min = ImVec2{all_bars_bb.Min.x, all_bars_bb.Min.y};              // Min=Upper-left
      bar_bb.Max = ImVec2{all_bars_bb.Min.x + bar_delta, all_bars_bb.Max.y};  // Max=Lower-right
      break;
  }

  // Render levels
  for (size_t i = 0; i < data_size; ++i) {
    // Render single level
    DrawBar(orientation, data[i], v_min, v_max, bar_bb, color_map, color_map_size, map_func);
    // calc next level bounding box
    switch (orientation) {
      case Orientation::Horizontal:
        bar_bb.Min.y = bar_bb.Max.y + bar_padding;  // Min=Upper-left
        bar_bb.Max.y = bar_bb.Min.y + bar_delta;    // Max=Lower-right
        break;
      case Orientation::Vertical:
        bar_bb.Min.x = bar_bb.Max.x + bar_padding;  // Min=Upper-left
        bar_bb.Max.x = bar_bb.Min.x + bar_delta;    // Max=Lower-right
        break;
    }
  }

  // Render label
  if (label_size.x > 0.0f) {
    switch (orientation) {
      case Orientation::Horizontal:
        ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y),
                          label);
        break;
      case Orientation::Vertical:
        ImGui::RenderText(ImVec2(frame_bb.Min.x + style.ItemInnerSpacing.x, frame_bb.Max.y + style.FramePadding.y),
                          label);
        break;
    }
  }

  return false;
}

bool AudioLevelMeter(const char *label, float const *data, size_t data_size, ImGuiMeterFlags flags, float v_min,
                     float v_max, ImVec2 size, ColorMapEntry<float> const *color_map, size_t color_map_size,
                     MapFunction<float> map_func) {
  if (color_map == nullptr) {
    color_map = kDefaultColorMapFloat.data();
    color_map_size = kDefaultColorMapFloat.size();
  }
  return BaseAudioLevelMeter(label, ImGuiDataType_Float, data, data_size, flags, v_min, v_max, size, color_map,
                             color_map_size, map_func);
}

bool AudioLevelMeter(const char *label, double const *data, size_t data_size, ImGuiMeterFlags flags, double v_min,
                     double v_max, ImVec2 size, ColorMapEntry<double> const *color_map, size_t color_map_size,
                     MapFunction<double> map_func) {
  if (color_map == nullptr) {
    color_map = kDefaultColorMapDouble.data();
    color_map_size = kDefaultColorMapDouble.size();
  }
  return BaseAudioLevelMeter(label, ImGuiDataType_Float, data, data_size, flags, v_min, v_max, size, color_map,
                             color_map_size, map_func);
}

}  // namespace ImGuiMeter
