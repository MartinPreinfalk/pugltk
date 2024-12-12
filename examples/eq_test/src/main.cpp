
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
    auto main_view = ui.AddView(pugltk::View::Parameter("EQ Test", 800, 600), [&show_demo, &input_level]() {
      bool p_open = true;
      ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize, ImGuiCond_Always);
      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_Always);
      ImGuiWindowFlags window_flags = 0;
      // window_flags |= ImGuiWindowFlags_NoTitleBar;
      window_flags |= ImGuiWindowFlags_NoScrollbar;
      //window_flags |= ImGuiWindowFlags_MenuBar;
      window_flags |= ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoResize;
      window_flags |= ImGuiWindowFlags_NoCollapse;
      window_flags |= ImGuiWindowFlags_NoNav;
      // window_flags |= ImGuiWindowFlags_NoBackground;
      // window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
      // window_flags |= ImGuiWindowFlags_UnsavedDocument;

      ImGui::Begin("foo", &p_open, window_flags);
      ImGui::DragFloat("input", &input_level);

      if(ImGui::Button("show_demo")) {
        show_demo = true;
      }
      ImGui::End();
      return p_open;
    });
    // show view
    cout << "show ui...\n";
    ui.Show();
    
    cout << "render ui...\n";
    while (ui.Render(0.04)) {
      if(show_demo) {
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