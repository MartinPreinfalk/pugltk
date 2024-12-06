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

namespace pugltk {

View::View(::pugl::World& world, ViewId const& view_id) : ::pugl::View{world}, view_id_(std::move(view_id)) {}

::pugl::Status View::Init(const std::string& title, PuglSpan const& width, PuglSpan const& height, bool resizable,
                          ::pugl::NativeView const& parent, ImGuiFrameFunction const& imgui_frame_function) {
  title_ = title;
  std::cerr << "Init view " << title_ << std::endl;
  setString(::pugl::StringHint::windowTitle, title.c_str());
  setSizeHint(::pugl::SizeHint::defaultSize, width, height);
  setSizeHint(::pugl::SizeHint::minSize, width, height);
  // setSizeHint(::pugl::SizeHint::maxSize, width, height);
  // setHint(::pugl::ViewHint::resizable, PUGL_FALSE);
  // setSizeHint(::pugl::SizeHint::minAspect, 1, 1);
  // setSizeHint(::pugl::SizeHint::maxAspect, 16, 9);
  setHint(::pugl::ViewHint::resizable, resizable ? PUGL_TRUE : PUGL_FALSE);
  setHint(::pugl::ViewHint::ignoreKeyRepeat, PUGL_TRUE);
  setEventHandler(*this);
  setBackend(puglGlBackend());
  setHint(::pugl::ViewHint::contextVersionMajor, 3);
  setHint(::pugl::ViewHint::contextVersionMinor, 3);
  setHint(::pugl::ViewHint::contextProfile, PUGL_OPENGL_CORE_PROFILE);
  if (imgui_frame_function) {
    SetImGuiFrameFunction(imgui_frame_function);
  }
  if (parent != ::pugl::NativeView{}) {
    setParentWindow(parent);
  }
  return realize();  // realizes view
}

void View::SetupImGuiStyle() {
  ImGuiIO& io = ImGui::GetIO();
  auto new_scale =
      ::puglGetScaleFactor(cobj());  // FIXME: at least on x11 the scale factor is only read on world init so
                                     // rescaling during runtime seems to be currently not supported by pugl
  std::cout << "scale_ = " << scale_ << std::endl;
  std::cout << "new_scale = " << new_scale << std::endl;
  if (new_scale != scale_) {  // first time and whenever it changes
    scale_ = new_scale;
    std::cout << "rescaling to scale_ = " << scale_ << std::endl;
    // just in case, copy the old style
    // ::ImGuiStyle prev_style = ::ImGui::GetStyle();

    // start with a fresh initialized structure for re-scaling as mentioned in comment of ImGuiStyle::ScaleAllSizes()
    // ::ImGui::GetStyle() = ImGuiStyle();

    // Setup Style (here we only use the default)
    ::ImGui::StyleColorsDark();

    // Setup Fonts
    // io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 10 * scale_);
    // io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 10 * scale_);
    // io.Fonts->AddFontFromFileTTF("Karla-Regular.ttf", 10 * scale_);
    // io.Fonts->AddFontFromFileTTF("ProggyClean.ttf", 10 * scale_);
    // io.Fonts->AddFontFromFileTTF("ProggyTiny.ttf", 10 * scale_);
    // io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 10 * scale_);
    // io.Fonts->GetTexDataAsRGBA32(...?)
    // io.Fonts->GetTexDataAsAlpha8(...?);

    // Setup style scaling
    ::ImGui::GetStyle().ScaleAllSizes(scale_);

    // Setup font scaling
    io.FontGlobalScale = scale_;
  }
}

::pugl::Status View::onEvent(const ::pugl::RealizeEvent& /*event*/) noexcept {
  std::cerr << "RealizeEvent view " << title_ << std::endl;
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
  ImGui::SetCurrentContext(imgui_ctx_);

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
    on_realize_event_function_();
  }

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::UnrealizeEvent& event) noexcept {
  std::cerr << "UnrealizeEvent view " << title_ << std::endl;
  if (on_unrealize_event_function_) {
    on_unrealize_event_function_();
  }
  // context active, can be used to destroy shader, textures, ...
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplPugl_Shutdown();
  ImGui::DestroyContext(imgui_ctx_);

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ConfigureEvent& event) noexcept {
  std::cerr << "ConfigureEvent view " << title_ << " "
            << "x: " << event.x << ", "
            << "y: " << event.y << ", "
            << "width: " << event.width << ", "
            << "height: " << event.height << ", "
            << "style: " << event.style << ", "
            << "type: " << event.type << ", "
            << "flags: " << event.flags << endl;

  // context active, but no drawing possible
  xpos_ = event.x;
  ypos_ = event.y;
  width_ = event.width;
  height_ = event.height;

  glViewport(0, 0, width_, height_);

  // // Setup Dear ImGui style
  // SetupImGuiStyle();  // FIXME: currently not used because (at least on x11) pugl does not support rescale during
  //                     // runtime

  if (on_configure_event_function_) {
    on_configure_event_function_(xpos_, ypos_, width_, height_);
  }

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::UpdateEvent& /*event*/) noexcept {
  // std::cerr << "UpdateEvent view " << title_ << std::endl;
  postRedisplay();  // request expose event on every update
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ExposeEvent& /*event*/) noexcept {
  // std::cerr << "ExposeEvent view " << title_ << std::endl;

  // Start the Dear ImGui frame
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplPugl_NewFrame();
  ImGui::NewFrame();

  // call client's render function
  if (imgui_frame_function_ && imgui_frame_function_()) {
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
  if (opengl_frame_function_ && opengl_frame_function_()) {
    close_flag_ = true;
  }

  // render imgui on top of it
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::CloseEvent& /*event*/) noexcept {
  std::cerr << "CloseEvent view " << title_ << std::endl;
  close_flag_ = true;
  if (on_close_event_function_) {
    on_close_event_function_();
  }
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::FocusInEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_FocusEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::FocusOutEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_FocusEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::KeyPressEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_KeyEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::KeyReleaseEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_KeyEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::TextEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_TextEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::PointerInEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_CrossingEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::PointerOutEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_CrossingEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ButtonPressEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_ButtonEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ButtonReleaseEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_ButtonEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::MotionEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_MotionEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ScrollEvent& event) noexcept {
  ImGui::SetCurrentContext(imgui_ctx_);
  ImGui_ImplPugl_ScrollEventHandler(cobj(), &event);
  return ::pugl::Status::success;
}

::pugl::Status View::onEvent(const ::pugl::ClientEvent& event) noexcept { return ::pugl::Status::success; }
::pugl::Status View::onEvent(const ::pugl::TimerEvent& event) noexcept { return ::pugl::Status::success; }
::pugl::Status View::onEvent(const ::pugl::LoopEnterEvent& event) noexcept { return ::pugl::Status::success; }
::pugl::Status View::onEvent(const ::pugl::LoopLeaveEvent& event) noexcept { return ::pugl::Status::success; }
::pugl::Status View::onEvent(const ::pugl::DataOfferEvent& event) noexcept { return ::pugl::Status::success; }
::pugl::Status View::onEvent(const ::pugl::DataEvent& event) noexcept { return ::pugl::Status::success; }

// // catch all for unused event types
// template <typename EventType>
// ::pugl::Status onEvent(const EventType& event) noexcept {
//   ImGui::SetCurrentContext(imgui_ctx_);
//   ImGui_ImplPugl_EventHandler(cobj(), (PuglEvent*)(&event));
//   return ::pugl::Status::success;
// }

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

}  // namespace oat::ui::pugl
