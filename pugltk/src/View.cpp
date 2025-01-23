#include "pugltk/View.h"

// clang-format off
#define PUGL_NO_INCLUDE_GL_H
#include "glad/glad.h"
#include <pugl/pugl.hpp>
#include <pugl/gl.h>
#include <imgui.h>
#include <imgui_impl_pugl.h>
#include <imgui_impl_opengl3.h>
// clang-format on

#include "pugltk/internal/trace_macro.h"

namespace pugltk {

View::Parameter::Parameter() {}

View::Parameter::Parameter(std::string title, ::pugl::NativeView parent) : title(std::move(title)), parent(parent) {}

View::Parameter::Parameter(std::string title, PuglSpan width, PuglSpan height, ::pugl::NativeView parent)
    : title(std::move(title)), default_width(width), default_height(height), parent(parent) {}

View::View(::pugl::World& world, ViewId const& view_id) : ::pugl::View{world}, view_id_(std::move(view_id)) {}

::pugl::Status View::Init(Parameter const& parameter, ImGuiFrameFunction const& imgui_frame_function) {
  scale_ = ::puglGetScaleFactor(cobj());
  parameter_ = parameter;
  setString(::pugl::StringHint::windowTitle, parameter_.title.c_str());
  setSizeHint(::pugl::SizeHint::defaultSize, parameter_.default_width * scale_, parameter_.default_height * scale_);
  setSizeHint(::pugl::SizeHint::minSize, parameter_.min_width * scale_, parameter_.min_height * scale_);
  // setSizeHint(::pugl::SizeHint::maxSize, width, height);
  // setHint(::pugl::ViewHint::resizable, PUGL_FALSE);
  // setSizeHint(::pugl::SizeHint::minAspect, 1, 1);
  // setSizeHint(::pugl::SizeHint::maxAspect, 16, 9);
  setHint(::pugl::ViewHint::resizable, parameter_.resizable ? PUGL_TRUE : PUGL_FALSE);
  setHint(::pugl::ViewHint::ignoreKeyRepeat, PUGL_TRUE);
  setEventHandler(*this);
  setBackend(puglGlBackend());
  setHint(::pugl::ViewHint::contextVersionMajor, 3);
  setHint(::pugl::ViewHint::contextVersionMinor, 3);
  setHint(::pugl::ViewHint::contextProfile, PUGL_OPENGL_CORE_PROFILE);
  if (imgui_frame_function) {
    SetImGuiFrameFunction(imgui_frame_function);
  }
  if (parameter_.parent != ::pugl::NativeView{}) {
    setParent(parameter_.parent);
  }
  return realize();  // realizes view
}

void View::Rescale() {
  auto new_scale =
      ::puglGetScaleFactor(cobj());  // FIXME: at least on x11 the scale factor is only read on world init so
                                     // rescaling during runtime seems to be currently not supported by pugl
  if (new_scale != scale_) {         // first time and whenever it changes
    LOG_TRACE("scale_ = " << scale_ << ", new_scale = " << new_scale);
    scale_ = new_scale;
    LOG_TRACE("rescaling to scale_ = " << scale_);
    SetupImGuiStyle();
  }
}

void View::SetupImGuiStyle() {
  // just in case, copy the old style
  // ::ImGuiStyle prev_style = ::ImGui::GetStyle();

  // start with a fresh initialized structure for re-scaling as mentioned in comment of ImGuiStyle::ScaleAllSizes()
  // ::ImGui::GetStyle() = ImGuiStyle();

  // Setup Style (here we only use the default)
  ::ImGui::StyleColorsDark();

  // Setup font and font scaling
  SetupFont();

  // Setup style scaling
  ::ImGui::GetStyle().ScaleAllSizes(scale_);
}

void View::SetFont(fonts::FontId const& font_id, size_t const& font_size) {
  parameter_.font_id = font_id;
  parameter_.font_size = font_size;
  SetupFont();
}

void View::SetFontId(fonts::FontId const& font_id) {
  parameter_.font_id = font_id;
  SetupFont();
}

void View::SetFontSize(size_t const& font_size) {
  parameter_.font_size = font_size;
  SetupFont();
}

void View::SetupFont() {
  SetContextCurrent();
  if (parameter_.font_id != fonts::FontId::kDefault) {
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fonts::GetCompressedTTFData(parameter_.font_id),
                                                         fonts::GetCompressedTTFSize(parameter_.font_id),
                                                         parameter_.font_size * scale_);
  } else {
    ImGui::GetIO().Fonts->ClearFonts();
    ImGui::GetIO().FontGlobalScale = scale_;
  }
}

void View::SetContextCurrent() {
  if (imgui_ctx_) ImGui::SetCurrentContext(imgui_ctx_);
  if (implot_ctx_) ImPlot::SetCurrentContext(implot_ctx_);
}

::pugl::Status View::onEvent(const ::pugl::RealizeEvent& /*event*/) noexcept {
  LOG_TRACE("RealizeEvent view " << parameter_.title);;
  // context active, can be used to create shader, textures, ...

  // init glad
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(puglGetProcAddress))) {
    // throw std::runtime_error("Failed to initialize GLAD");
    return ::pugl::Status::unknownError;
  }

  // init imgui
  constexpr char glsl_version[] = "#version 330";

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  imgui_ctx_ = ImGui::CreateContext();
  SetContextCurrent();
  implot_ctx_ = ImPlot::CreateContext();
  SetContextCurrent();

  LOG_TRACE("Platform_LocaleDecimalPoint: " << ImGui::GetPlatformIO().Platform_LocaleDecimalPoint);
  ImGui::GetPlatformIO().Platform_LocaleDecimalPoint = *localeconv()->decimal_point;
  LOG_TRACE("Platform_LocaleDecimalPoint: " << ImGui::GetPlatformIO().Platform_LocaleDecimalPoint);

  // ImGuiIO& io = ImGui::GetIO();
  // (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  // Setup Platform/Renderer bindings
  // ImGui_ImplPugl_InitForOpenGL(cobj(), true);
  ImGui_ImplPugl_InitForOpenGL(cobj());
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Setup Dear ImGui style
  SetupImGuiStyle();

  if (on_realize_event_function_) {
    on_realize_event_function_(*this);
  }

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::UnrealizeEvent& /*event*/) noexcept {
  LOG_TRACE("UnrealizeEvent view " << parameter_.title);
  if (on_unrealize_event_function_) {
    on_unrealize_event_function_(*this);
  }
  // context active, can be used to destroy shader, textures, ...
  SetContextCurrent();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplPugl_Shutdown();

  ImPlot::DestroyContext(implot_ctx_);
  implot_ctx_ = nullptr;
  ImGui::DestroyContext(imgui_ctx_);
  imgui_ctx_ = nullptr;

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ConfigureEvent& event) noexcept {
  LOG_TRACE("ConfigureEvent view " << parameter_.title << " "
                                   << "x: " << event.x << ", "
                                   << "y: " << event.y << ", "
                                   << "width: " << event.width << ", "
                                   << "height: " << event.height << ", "
                                   << "style: " << event.style << ", "
                                   << "type: " << event.type << ", "
                                   << "flags: " << event.flags);

  // context active, but no drawing possible
  xpos_ = event.x;
  ypos_ = event.y;
  width_ = event.width;
  height_ = event.height;

  glViewport(0, 0, width_, height_);

  // Setup Dear ImGui style
  // Rescale();  // FIXME: currently not used because (at least on x11) pugl does not support rescale during runtime

  if (on_configure_event_function_) {
    on_configure_event_function_(xpos_, ypos_, width_, height_);
  }

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::UpdateEvent& /*event*/) noexcept {
  // LOG_TRACE("UpdateEvent view " << title_);
  postRedisplay();  // request expose event on every update
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ExposeEvent& /*event*/) noexcept {
  // LOG_TRACE("ExposeEvent view " << title_);

  // Start the Dear ImGui frame
  SetContextCurrent();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplPugl_NewFrame();
  ImGui::NewFrame();

  // call client's render function
  if (imgui_frame_function_ && !imgui_frame_function_(*this)) {
    close_flag_ = true;
  }

  // Finish gui Frame
  ImGui::Render();

  // glViewport(0, 0, width_, height_);

  // bind default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // clear frame
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // render client's opengl calls if any
  if (opengl_frame_function_ && !opengl_frame_function_(*this)) {
    close_flag_ = true;
  }

  // render imgui on top of it
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::CloseEvent& /*event*/) noexcept {
  LOG_TRACE("CloseEvent view " << parameter_.title);
  close_flag_ = true;
  if (on_close_event_function_) {
    on_close_event_function_();
  }
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::FocusInEvent& event) noexcept {
  SetContextCurrent();
  ImGui_ImplPugl_FocusEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::FocusOutEvent& event) noexcept {
  SetContextCurrent();
  ImGui_ImplPugl_FocusEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::KeyPressEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_KeyEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::KeyReleaseEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_KeyEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::TextEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_TextEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::PointerInEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_CrossingEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::PointerOutEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_CrossingEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ButtonPressEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_ButtonEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ButtonReleaseEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_ButtonEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::MotionEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_MotionEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ScrollEvent& event) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  SetContextCurrent();
  ImGui_ImplPugl_ScrollEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ClientEvent& /*event*/) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::TimerEvent& /*event*/) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::LoopEnterEvent& /*event*/) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::LoopLeaveEvent& /*event*/) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::DataOfferEvent& /*event*/) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::DataEvent& /*event*/) noexcept {
  LOG_TRACE(__PRETTY_FUNCTION__);
  return ::pugl::Status::success;
}

void View::SetImGuiFrameFunction(ImGuiFrameFunction const& imgui_frame_function) {
  imgui_frame_function_ = imgui_frame_function;
}

void View::SetOpenGlFrameFunction(OpenGlFrameFunction const& opengl_frame_function) {
  opengl_frame_function_ = opengl_frame_function;
}

void View::SetOnRealizeEventFunction(OnRealizeEventFunction const& on_realize_event_function) {
  on_realize_event_function_ = on_realize_event_function;
}

void View::SetOnUnrealizeEventFunction(OnUnrealizeEventFunction const& on_unrealize_event_function) {
  on_unrealize_event_function_ = on_unrealize_event_function;
}

void View::SetOnConfigureEventFunction(OnConfigureEventFunction const& on_configure_event_function) {
  on_configure_event_function_ = on_configure_event_function;
}

void View::SetOnCloseEventFunction(OnCloseEventFunction const& on_close_event_function) {
  on_close_event_function_ = on_close_event_function;
}

}  // namespace pugltk
