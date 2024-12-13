#include "pugltk/Ui.h"

#include <algorithm>
#include <experimental/map>
#include <iostream>
#include <stdexcept>

namespace pugltk {

Ui::Ui() = default;

bool Ui::Init(::pugl::WorldType const& world_type) {
  if (pugl_world_ != nullptr) {
    std::cerr << "pugl_world_ already initialized!" << std::endl;
    return false;
  }
  next_id_ = 0;
  pugl_world_ = std::make_unique<::pugl::World>(world_type);
  if (!pugl_world_) {
    std::cerr << "could not create pugl_world_ instance!" << std::endl;
    return false;
  }
  if (pugl_world_->setString(::pugl::StringHint::className, "oat::ui::pugl::World") != ::pugl::Status::success) {
    std::cerr << "could not set string hint className on pugl_world_ instance!" << std::endl;
    return false;
  }
  return true;
}

std::shared_ptr<View> Ui::AddView(View::Parameter const& parameter,
                                  View::ImGuiFrameFunction const& main_view_imgui_frame_function) {
  if (pugl_world_ == nullptr) {
    std::cerr << "pugl_world_ not initialized!" << std::endl;
    return nullptr;
  }
  auto view = std::make_shared<View>(*pugl_world_, next_id_++);
  if (view == nullptr) {
    std::cerr << "could not create View instance!" << std::endl;
    return nullptr;
  }
  auto status = view->Init(parameter, main_view_imgui_frame_function);
  if (status != ::pugl::Status::success) {
    std::cerr << "could not initialize View instance - " << ::pugl::strerror(status) << std::endl;
    return nullptr;
  }
  if (views_.empty()) {
    main_view_ = view;
  } else {
    view->show(::pugl::ShowCommand::raise);
  }
  views_.insert(std::pair<size_t, std::shared_ptr<View>>(view->Id(), view));
  return view;
}

bool Ui::RemoveView(ViewId const& view_id) {
  auto view_it = views_.find(view_id);
  if (view_it == views_.end()) {
    std::cerr << "could not remove view with id " << view_id << " because it does not exist!" << std::endl;
    return false;
  }
  auto view = view_it->second;
  std::cerr << "hide view " << view_id << std::endl;
  view->hide();

  views_.erase(view_it);
  if (view == main_view_ && !views_.empty()) {
    main_view_.reset();
  }
  return true;
}

void Ui::Show() {
  for (auto& view : views_) {
    view.second->show(::pugl::ShowCommand::raise);
  }
}

void Ui::Hide() {
  for (auto& view : views_) {
    view.second->hide();
  }
}

bool Ui::Render(double const& delta_t) {
  auto status = pugl_world_->update(delta_t);
  if (status != ::pugl::Status::success) {
    std::cerr << "world update returned pugl status error - " << ::pugl::strerror(status) << std::endl;
    return false;
  }

  // check if main window was closed
  if (!main_view_ || (main_view_ && main_view_->CloseFlag())) {
    std::cerr << "main window was closed" << std::endl;
    return false;
  }

  // early exit
  if (std::none_of(views_.begin(), views_.end(), [](auto const& view) { return view.second->CloseFlag(); })) {
    return true;
  }

  // seems that one or more subviews have been closed.
  for (auto& view : views_) {
    if (view.second->CloseFlag()) {
      view.second->hide();
      view.second->unrealize();
    }
  }
  std::experimental::erase_if(views_, [](auto const& view) { return view.second->CloseFlag(); });
  return true;
}

void Ui::DeInit() {
  for (auto& view : views_) {
    view.second->unrealize();
  }
  views_.clear();
  main_view_.reset();
  pugl_world_.reset();
}

std::shared_ptr<View> Ui::GetView(ViewId const& view_id) {
  if (views_.find(view_id) == views_.end()) {
    return nullptr;
  }
  return views_.at(view_id);
}

bool Ui::SetMainView(ViewId const& view_id) {
  auto view = GetView(view_id);
  if (view == nullptr) {
    return false;
  }
  main_view_ = view;
  return true;
}

}  // namespace pugltk
