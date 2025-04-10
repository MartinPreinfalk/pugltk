#pragma once

#include <imgui.h>

namespace ImGuiUtils {

/**
 * from https://github.com/ocornut/imgui/issues/1901#issue-335266223
 * credits to https://github.com/zfedoran
 */
bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);

/**
 * from https://github.com/ocornut/imgui/issues/1901#issue-335266223
 * credits to https://github.com/zfedoran
 */
bool Spinner(const char* label, float radius, int thickness, const ImU32& color);

/**
 * from https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
 * credits to https://github.com/alexsr
 */
void LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color,
                            const ImVec4& backdrop_color, const int circle_count, const float speed);

void ShowDemoWindow(bool* p_open = nullptr);  // create Demo window.

}  // namespace ImGuiUtils
