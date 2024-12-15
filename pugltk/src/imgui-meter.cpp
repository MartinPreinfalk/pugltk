#include "imgui-meter.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdlib>

#define IMGUIKNOBS_PI 3.14159265358979323846f

namespace ImGuiMeter {

struct color_set {
  ImColor base;
  ImColor hovered;
  ImColor active;

  color_set(ImColor base, ImColor hovered, ImColor active) : base(base), hovered(hovered), active(active) {}

  color_set(ImColor color) {
    base = color;
    hovered = color;
    active = color;
  }
};

namespace detail {

void draw_arc(ImVec2 center, float radius, float start_angle, float end_angle, float thickness, ImColor color) {
  auto *draw_list = ImGui::GetWindowDrawList();

  draw_list->PathArcTo(center, radius, start_angle, end_angle);
  draw_list->PathStroke(color, 0, thickness);
}

template <typename DataType>
struct Meter {
  float radius;
  bool value_changed;
  ImVec2 center;
  bool is_active;
  bool is_hovered;
  float angle_min;
  float angle_max;
  float t;
  float angle;
  float angle_cos;
  float angle_sin;

  Meter(const char *_label, ImGuiDataType data_type, DataType *p_value, DataType v_min, DataType v_max, float speed,
       float _radius, const char *format, ImGuiKnobFlags flags, float _angle_min, float _angle_max) {
    radius = _radius;
    t = ((float)*p_value - v_min) / (v_max - v_min);
    auto screen_pos = ImGui::GetCursorScreenPos();

    // Handle dragging
    ImGui::InvisibleButton(_label, {radius * 2.0f, radius * 2.0f});

    // Handle drag: if DragVertical or DragHorizontal flags are set, only the given direction is
    // used, otherwise use the drag direction with the highest delta
    ImGuiIO &io = ImGui::GetIO();
    bool drag_vertical =
        !(flags & ImGuiKnobFlags_DragHorizontal) &&
        (flags & ImGuiKnobFlags_DragVertical || ImAbs(io.MouseDelta[ImGuiAxis_Y]) > ImAbs(io.MouseDelta[ImGuiAxis_X]));

    auto gid = ImGui::GetID(_label);
    value_changed = ImGui::DragBehavior(gid, data_type, p_value, speed, &v_min, &v_max, format,
                                        drag_vertical ? ImGuiSliderFlags_Vertical : 0);

    angle_min = _angle_min < 0 ? IMGUIKNOBS_PI * 0.75f : _angle_min;
    angle_max = _angle_max < 0 ? IMGUIKNOBS_PI * 2.25f : _angle_max;

    center = {screen_pos[0] + radius, screen_pos[1] + radius};
    is_active = ImGui::IsItemActive();
    is_hovered = ImGui::IsItemHovered();
    angle = angle_min + (angle_max - angle_min) * t;
    angle_cos = cosf(angle);
    angle_sin = sinf(angle);
  }

  void draw_dot(float size, float radius, float angle, color_set color, bool filled, int segments) {
    auto dot_size = size * this->radius;
    auto dot_radius = radius * this->radius;

    ImGui::GetWindowDrawList()->AddCircleFilled(
        {center[0] + cosf(angle) * dot_radius, center[1] + sinf(angle) * dot_radius}, dot_size,
        is_active ? color.active : (is_hovered ? color.hovered : color.base), segments);
  }

  void draw_tick(float start, float end, float width, float angle, color_set color) {
    auto tick_start = start * radius;
    auto tick_end = end * radius;
    auto angle_cos = cosf(angle);
    auto angle_sin = sinf(angle);

    ImGui::GetWindowDrawList()->AddLine({center[0] + angle_cos * tick_end, center[1] + angle_sin * tick_end},
                                        {center[0] + angle_cos * tick_start, center[1] + angle_sin * tick_start},
                                        is_active ? color.active : (is_hovered ? color.hovered : color.base),
                                        width * radius);
  }

  void draw_circle(float size, color_set color, bool filled, int segments) {
    auto circle_radius = size * radius;

    ImGui::GetWindowDrawList()->AddCircleFilled(center, circle_radius,
                                                is_active ? color.active : (is_hovered ? color.hovered : color.base));
  }

  void draw_arc(float radius, float size, float start_angle, float end_angle, color_set color) {
    auto track_radius = radius * this->radius;
    auto track_size = size * this->radius * 0.5f + 0.0001f;

    detail::draw_arc(center, track_radius, start_angle, end_angle, track_size,
                     is_active ? color.active : (is_hovered ? color.hovered : color.base));
  }
};

template <typename DataType>
Meter<DataType> CreateMeter(const char *label, ImGuiDataType data_type, DataType const *values_db, size_t n_values,
                         DataType v_min_db, DataType v_max_db, const char *format, ImVec2 size, ImGuiMeterFlags flags) {
  ImGui::PushID(label);
  auto width = size == 0 ? ImGui::GetTextLineHeight() * 4.0f : size * ImGui::GetIO().FontGlobalScale;
  ImGui::PushItemWidth(width);

  ImGui::BeginGroup();

  // There's an issue with `SameLine` and Groups, see
  // https://github.com/ocornut/imgui/issues/4190. This is probably not the best
  // solution, but seems to work for now
  ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = 0;

  // Draw title
  if (!(flags & ImGuiMeterFlags_NoTitle)) {
    auto title_size = ImGui::CalcTextSize(label, NULL, false, width);

    // Center title
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - title_size[0]) * 0.5f);

    ImGui::Text("%s", label);
  }

  // Draw meter
  Meter<DataType> k(label, data_type, values_db, n_values, v_min, v_max, width * 0.5f, format, flags);

  // Draw tooltip
  if (flags & ImGuiKnobFlags_ValueTooltip &&
      (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemActive())) {
    ImGui::BeginTooltip();
    ImGui::Text(format, *p_value);
    ImGui::EndTooltip();
  }

  // Draw input
  if (!(flags & ImGuiKnobFlags_NoInput)) {
    auto changed = ImGui::DragScalar("###knob_drag", data_type, p_value, speed, &v_min, &v_max, format);
    if (changed) {
      k.value_changed = true;
    }
  }

  ImGui::EndGroup();
  ImGui::PopItemWidth();
  ImGui::PopID();

  return k;
}

color_set GetPrimaryColorSet() {
  auto *colors = ImGui::GetStyle().Colors;

  return {colors[ImGuiCol_ButtonActive], colors[ImGuiCol_ButtonHovered], colors[ImGuiCol_ButtonHovered]};
}

color_set GetSecondaryColorSet() {
  auto *colors = ImGui::GetStyle().Colors;
  auto active = ImVec4(colors[ImGuiCol_ButtonActive].x * 0.5f, colors[ImGuiCol_ButtonActive].y * 0.5f,
                       colors[ImGuiCol_ButtonActive].z * 0.5f, colors[ImGuiCol_ButtonActive].w);

  auto hovered = ImVec4(colors[ImGuiCol_ButtonHovered].x * 0.5f, colors[ImGuiCol_ButtonHovered].y * 0.5f,
                        colors[ImGuiCol_ButtonHovered].z * 0.5f, colors[ImGuiCol_ButtonHovered].w);

  return {active, hovered, hovered};
}

color_set GetTrackColorSet() {
  auto *colors = ImGui::GetStyle().Colors;

  return {colors[ImGuiCol_Button], colors[ImGuiCol_Button], colors[ImGuiCol_Button]};
}
}  // namespace detail

template <typename DataType>
bool BaseAudioLevelMeter(const char *label, ImGuiDataType data_type, DataType const *values_db, size_t n_values,
                         DataType v_min_db, DataType v_max_db, const char *format, ImGuiMeterVariant variant,
                         ImVec2 size, ImGuiMeterFlags flags) {
  auto meter =
      detail::CreateMeter(label, data_type, values_db, n_values, v_min_db, v_max_db, format, size, flags);

  switch (variant) {
    case ImGuiMeterVariant_Bars: {
      meter.draw_bars(0.9f, detail::GetPrimaryColorSet());
      break;
    }
  }
  return knob.value_changed;
}

bool AudioLevelMeter(const char *label, float const *values_db, size_t n_values, float v_min_db, float v_max_db,
                     const char *format, ImGuiMeterVariant variant, ImVec2 size, ImGuiMeterFlags flags) {
  return BaseAudioLevelMeter(label, ImGuiDataType_Float, values_db, n_values, v_min_db, v_max_db, format, variant, size,
                             flags);
}

}  // namespace ImGuiMeter
