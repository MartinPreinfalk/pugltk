#pragma once

#include <imgui.h>
#include <cstdint>
#include <string>
#include <chrono>
#include <atomic>
#include <functional>
#include <iostream>
#include <pugl/pugl.hpp>
#include <thread>

#include "glad/glad.h"
#include "fonts/fonts.h"

using namespace std;
using namespace std::chrono_literals;

namespace pugltk {

using ViewId = size_t;

class View : public ::pugl::View {
 public:
  struct Parameter {
    std::string title = "Untitled";
    PuglSpan default_width = 640, default_height = 480;
    PuglSpan min_width = 0, min_height = 0;
    bool resizable = true;
    ::pugl::NativeView parent = ::pugl::NativeView{};
    fonts::FontId font_id = fonts::DefaultFontId;
    size_t font_size = fonts::kDefaultFontSize;
    Parameter();
    Parameter(std::string title, ::pugl::NativeView parent = ::pugl::NativeView{});
    Parameter(std::string title, PuglSpan width, PuglSpan height,
                       ::pugl::NativeView parent = ::pugl::NativeView{});
  };

  using ImGuiFrameFunction = std::function<bool()>;
  using OpenGlFrameFunction = std::function<bool()>;
  using OnRealizeEventFunction = std::function<void()>;
  using OnUnrealizeEventFunction = std::function<void()>;
  using OnConfigureEventFunction =
      std::function<void(PuglCoord const& xpos, PuglCoord const& ypos, PuglSpan const& width, PuglSpan const& height)>;
  using OnCloseEventFunction = std::function<void()>;

  explicit View(::pugl::World& world, ViewId const& view_id);

  ViewId const& Id() const { return view_id_; }

  ::pugl::Status Init(Parameter const& parameter = Parameter{},
                      ImGuiFrameFunction const& imgui_frame_function = nullptr);

  // setters for client code functions
  void SetImGuiFrameFunction(ImGuiFrameFunction const& imgui_frame_function);
  void SetOpenGlFrameFunction(OpenGlFrameFunction const& opengl_frame_function);
  void SetOnRealizeEventFunction(OnRealizeEventFunction const& on_realize_event_function);
  void SetOnUnrealizeEventFunction(OnUnrealizeEventFunction const& on_unrealize_event_function);
  void SetOnConfigureEventFunction(OnConfigureEventFunction const& on_configure_event_function);
  void SetOnCloseEventFunction(OnCloseEventFunction const& on_close_event_function);
  
  void SetFont(fonts::FontId const& font_id, size_t const& font_size);
  void SetFontId(fonts::FontId const& font_id);
  void SetFontSize(size_t const& font_size);

  Parameter const& GetParameter() const {return parameter_; }
  bool CloseFlag() const { return close_flag_; }
  PuglCoord XPos() const { return xpos_; }
  PuglCoord YPos() const { return ypos_; }
  PuglSpan Width() const { return width_; }
  PuglSpan Height() const { return height_; }
  double Scale() const { return scale_; }

  // event handlers used by pugl
  ::pugl::Status onEvent(const ::pugl::RealizeEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::UnrealizeEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::ConfigureEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::UpdateEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::ExposeEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::CloseEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::FocusInEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::FocusOutEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::KeyPressEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::KeyReleaseEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::TextEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::PointerInEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::PointerOutEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::ButtonPressEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::ButtonReleaseEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::MotionEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::ScrollEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::ClientEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::TimerEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::LoopEnterEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::LoopLeaveEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::DataOfferEvent& event) noexcept;
  ::pugl::Status onEvent(const ::pugl::DataEvent& event) noexcept;

  /// @copydoc puglRealize
  pugl::Status unrealize() noexcept { return static_cast<pugl::Status>(puglUnrealize(cobj())); }

 private:
  const ViewId view_id_ = 0;
  Parameter parameter_ = Parameter{};
  std::atomic<bool> close_flag_ = false;
  PuglCoord xpos_ = 0, ypos_ = 0;
  PuglSpan width_ = 0, height_ = 0;
  ImGuiContext* imgui_ctx_ = nullptr;
  double scale_ = std::numeric_limits<double>::quiet_NaN();
  ImGuiFrameFunction imgui_frame_function_;
  OpenGlFrameFunction opengl_frame_function_;
  OnRealizeEventFunction on_realize_event_function_;
  OnUnrealizeEventFunction on_unrealize_event_function_;
  OnConfigureEventFunction on_configure_event_function_;
  OnCloseEventFunction on_close_event_function_;

  void Rescale();
  void SetupImGuiStyle();
  void SetupFont();
};

}  // namespace pugltk
