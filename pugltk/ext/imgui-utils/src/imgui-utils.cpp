#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui-utils/imgui-utils.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdlib>

using namespace ImGui;

namespace ImGuiUtils {

/**
 * from https://github.com/ocornut/imgui/issues/1901#issue-335266223
 */
bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
  ImGuiWindow* window = GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID(label);

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 size = size_arg;
  size.x -= style.FramePadding.x * 2;

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id)) return false;

  // Render
  const float circleStart = size.x * 0.7f;
  const float circleEnd = size.x;
  const float circleWidth = circleEnd - circleStart;

  window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
  window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

  const float t = g.Time;
  const float r = size.y / 2;
  const float speed = 1.5f;

  const float a = speed * 0;
  const float b = speed * 0.333f;
  const float c = speed * 0.666f;

  const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
  const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
  const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

  window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
  window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
  window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
  return true;
}

/**
 * from https://github.com/ocornut/imgui/issues/1901#issue-335266223
 */
bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
  ImGuiWindow* window = GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID(label);

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id)) return false;

  // Render
  window->DrawList->PathClear();

  int num_segments = 30;
  int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

  const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
  const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

  const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

  for (int i = 0; i < num_segments; i++) {
    const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
    window->DrawList->PathLineTo(
        ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius));
  }

  window->DrawList->PathStroke(color, false, thickness);
  return true;
}

/**
 * from https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
 * credits to https://github.com/alexsr
 */
void LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color,
                            const ImVec4& backdrop_color, const int circle_count, const float speed) {
  ImGuiWindow* window = GetCurrentWindow();
  if (window->SkipItems) {
    return;
  }

  ImGuiContext& g = *GImGui;
  const ImGuiID id = window->GetID(label);

  const ImVec2 pos = window->DC.CursorPos;
  const float circle_radius = indicator_radius / 15.0f;
  const float updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
  const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
  ItemSize(bb);
  if (!ItemAdd(bb, id)) {
    return;
  }
  const float t = g.Time;
  const auto degree_offset = 2.0f * IM_PI / circle_count;
  for (int i = 0; i < circle_count; ++i) {
    const auto x = updated_indicator_radius * std::sin(degree_offset * i);
    const auto y = updated_indicator_radius * std::cos(degree_offset * i);
    const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
    ImVec4 color;
    color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
    color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
    color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
    color.w = 1.0f;
    window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y),
                                      circle_radius + growth * circle_radius, GetColorU32(color));
  }
}

}  // namespace ImGuiUtils
