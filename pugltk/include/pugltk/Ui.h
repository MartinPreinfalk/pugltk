#pragma once

#include <map>
#include <memory>
#include <pugl/pugl.hpp>

#include "pugltk/View.h"

namespace pugltk {

class Ui {
 public:
  using ViewId = size_t;
  Ui();
  bool Init(::pugl::WorldType const& world_type);
  std::shared_ptr<View> AddView(View::Parameter const& parameter = View::Parameter{},
                                View::ImGuiFrameFunction const& main_view_imgui_frame_function = nullptr);
  bool RemoveView(ViewId const& view_id);
  void Show();
  void Hide();
  bool Render(double const& delta_t);
  void DeInit();
  std::shared_ptr<View> GetView(ViewId const& view_id);
  bool SetMainView(ViewId const& view_id);
  std::shared_ptr<View> GetMainView() const { return main_view_; }

 private:
  std::unique_ptr<::pugl::World> pugl_world_;
  std::shared_ptr<View> main_view_;
  std::map<size_t, std::shared_ptr<View>> views_;
  ::pugl::WorldType world_type_;
  ViewId next_id_;
  bool hidden_ = true;
  bool CleanUpAndDecideIfWeWantToContinue();
};

}  // namespace pugltk
