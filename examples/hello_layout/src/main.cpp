
#include <iostream>

#include "pugltk/Ui.h"

using namespace std;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
  bool show_demo = false;
  float input_level = 0.0f;
  try {
    cout << "create ui...\n";
    pugltk::Ui ui;
    cout << "init ui...\n";
    ui.Init(::pugl::WorldType::program);
    cout << "init view...\n";
    auto main_view = ui.AddView(pugltk::View::Parameter("Hello Layout", 800, 600), [&show_demo, &input_level]() {
      bool p_open = true;
      ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize, ImGuiCond_Always);
      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_Always);
      ImGuiWindowFlags window_flags = 0;
      // window_flags |= ImGuiWindowFlags_NoTitleBar;
      window_flags |= ImGuiWindowFlags_NoScrollbar;
      // window_flags |= ImGuiWindowFlags_MenuBar;
      window_flags |= ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoResize;
      window_flags |= ImGuiWindowFlags_NoCollapse;
      window_flags |= ImGuiWindowFlags_NoNav;
      // window_flags |= ImGuiWindowFlags_NoBackground;
      // window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
      // window_flags |= ImGuiWindowFlags_UnsavedDocument;

      ImGui::Begin("foo", &p_open, window_flags);

      // Plot as lines and plot as histogram
        // static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ContextMenuInBody;
            // EditTableSizingFlags(&sizing_policy_flags[table_n]);

            // To make it easier to understand the different sizing policy,
            // For each policy: we display one table where the columns have equal contents width,
            // and one where the columns have different contents width.
            // if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_SizingFixedFit))
            // {
            //     for (int row = 0; row < 3; row++)
            //     {
            //         ImGui::TableNextRow();
            //         ImGui::TableNextColumn(); ImGui::Text("Oh dear");
            //         ImGui::TableNextColumn(); ImGui::Text("Oh dear");
            //         ImGui::TableNextColumn(); ImGui::Text("Oh dear");
            //     }
            //     ImGui::EndTable();
            // }
      if (ImGui::BeginTable("upper", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("##A", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##B", ImGuiTableColumnFlags_WidthFixed, 200.0f);

        ImGui::TableNextRow(ImGuiTableRowFlags_None, ImGui::GetMainViewport()->WorkSize.y /2);

        ImGui::TableNextColumn();
        static float arr[] = {0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f};
        ImGui::Button("Foo1");
        // ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(600.0, 400.0));      
        ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));      
        
        ImGui::TableNextColumn();
        ImGui::BeginGroup();
        ImGui::Button("Foo");
        ImGui::Text("Input");
        ImGui::DragFloat("##input_top", &input_level, 0.001f, -10.0f, 10.0f, "%.3f",
                            ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndGroup();
        ImGui::EndTable();
      }

      ImGui::Columns(8, "bottom", false);
      for (size_t i = 0; i < 8; ++i) {
        ImGui::Button(("Fooo" + std::to_string(i)).c_str());
        ImGui::DragFloat(("input_bottom_1_" + std::to_string(i)).c_str(), &input_level, 0.001f, -10.0f, 10.0f, "%.3f",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat(("input_bottom_2_" + std::to_string(i)).c_str(), &input_level, 0.001f, -10.0f, 10.0f, "%.3f",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat(("input_bottom_3_" + std::to_string(i)).c_str(), &input_level, 0.001f, -10.0f, 10.0f, "%.3f",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat(("input_bottom_4_" + std::to_string(i)).c_str(), &input_level, 0.001f, -10.0f, 10.0f, "%.3f",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::Button(("Foooo" + std::to_string(i)).c_str());
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
      if (ImGui::Button("show_demo")) {
        show_demo = true;
      }

      // ImGui::VSliderFloat("##v", ImVec2(18, 160), &input_level, 0.0f, 1.0f, "");

      ImGui::End();
      return p_open;
    });
    // show view
    cout << "show ui...\n";
    ui.Show();

    cout << "render ui...\n";
    while (ui.Render(0.04)) {
      if (show_demo) {
        show_demo = false;
        auto demo = ui.AddView(pugltk::View::Parameter("Demo", 800, 600), []() {
          bool p_open = true;
          ImGui::ShowDemoWindow(&p_open);
          cout << p_open << endl;
          return p_open;
        });
      }
    }
    cout << "hide ui...\n";
    ui.Hide();
    cout << "deinit ui...\n";
    ui.DeInit();
    cout << "terminate\n";
    return 0;
  } catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
  } catch (...) {
    std::cerr << "unkown exception caught!" << std::endl;
  }
  return 1;
}