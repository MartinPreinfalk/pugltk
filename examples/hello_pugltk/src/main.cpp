
#include <iostream>

#include "pugltk/Ui.h"

using namespace std;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
  try {
    int foo = 42;
    int bar = 42;
    bool create_view = false;
    cout << "create ui...\n";
    pugltk::Ui ui;
    cout << "init ui...\n";
    ui.Init(::pugl::WorldType::program);
    cout << "init view...\n";
    auto main_view = ui.AddView(pugltk::View::Parameter("Hello ImGui Pugl UI", 800, 600), [&foo, &create_view](::pugltk::View const& /*view*/) {
      ImGui::SetWindowFontScale(1.0);
      if (ImGui::Button("foo++")) {
        foo++;
      }
      if (ImGui::Button("create view")) {
        create_view = true;
      }
      if (ImGui::Button("Close")) {
        return false;
      }
      return true;
    });
    double r = 0, g = 0, b = 0;
    main_view->SetOpenGlFrameFunction([&r, &g, &b](::pugltk::View const& /*view*/) {
      r += 0.01;
      g += 0.011;
      b += 0.012;
      if (r > 1) r = 0;
      if (g > 1) g = 0;
      if (b > 1) b = 0;
      glClearColor(r, g, b, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      return true;
    });

    // show view
    cout << "show ui...\n";
    ui.Show();
    cout << "render ui...\n";
    while (ui.Render(0.04)) {
      std::cout << foo << " - " << bar << std::endl;
      if (create_view) {
        std::cout << "creating view..." << std::endl;
        auto params = pugltk::View::Parameter("Hello ImGui Pugl UI Bar", 800, 600);
        ui.AddView(params, [&bar](::pugltk::View const& /*view*/) {
          if (ImGui::Button("bar++")) {
            bar++;
          }
          if (ImGui::Button("Close")) {
            return false;
          }
          return true;
        });
        create_view = false;
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