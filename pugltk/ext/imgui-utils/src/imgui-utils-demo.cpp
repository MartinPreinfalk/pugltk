#include <imgui.h>

#include "imgui-utils/imgui-utils.h"

namespace ImGuiUtils {
void ShowDemoWindow(bool* p_open) {
  if (ImGui::Begin("imgui-utils-demo", p_open)) {
    const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);
    ImGuiUtils::Spinner("##spinner", 15, 6, col);
    ImGuiUtils::BufferingBar("##buffer_bar", 0.7f, ImVec2(400, 6), bg, col);
    ImGuiUtils::LoadingIndicatorCircle("##loading_indicator_circle", 100, ImGui::GetStyleColorVec4(ImGuiCol_Text),
                                       ImGui::GetStyleColorVec4(ImGuiCol_WindowBg), 10, 2);
    ImGui::End();
  }
}

}  // namespace ImGuiUtils
