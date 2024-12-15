cmake_minimum_required(VERSION 3.23)
project(pugltk VERSION 1.0)

include(cmake/install_cmake_config_macro.cmake)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# glad
target_sources(${PROJECT_NAME}
  PRIVATE
    ext/glad/src/glad/glad.c
  PUBLIC
    FILE_SET glad_headers 
    TYPE HEADERS
    BASE_DIRS
      ext/glad/include
    FILES
      ext/glad/include/glad/glad.h
      ext/glad/include/KHR/khrplatform.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>  
)

# pugl
if (UNIX AND NOT APPLE)
    set(PUGL_X11 ON)
    message(STATUS "platform unix was detected")
else()
    message(FATAL_ERROR "No supported platform was detected")
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
    ext/pugl/src/common.c
    ext/pugl/src/internal.c
  PUBLIC
    FILE_SET pugl_headers 
    TYPE HEADERS
    BASE_DIRS
      ext/pugl/include
    FILES
      ext/pugl/include/pugl/pugl.h
      ext/pugl/include/pugl/attributes.h
  PUBLIC
    FILE_SET pugl_cpp_headers 
    TYPE HEADERS
    BASE_DIRS
      ext/pugl/bindings/cpp/include
    FILES
      ext/pugl/bindings/cpp/include/pugl/pugl.hpp
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pugl/include> 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pugl/bindings/cpp/include> 
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pugl/src> 
)

if(PUGL_X11)
message(STATUS "pugl platform x11 and backend gl")
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

target_sources(${PROJECT_NAME}
  PRIVATE
    ext/pugl/src/x11_gl.c
    ext/pugl/src/x11.c
  PUBLIC
    FILE_SET pugl_platform_headers 
    TYPE HEADERS
    BASE_DIRS
      ext/pugl/include
    FILES
      ext/pugl/include/pugl/gl.h
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    $<BUILD_INTERFACE:${X11_X11_INCLUDE_PATH}> 
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    X11::X11
    X11::Xcursor
    X11::Xrandr
    X11::Xext
    OpenGL::GL
)

endif()

# imgui
target_sources(${PROJECT_NAME}
  PRIVATE
    ext/imgui/imgui_demo.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/imgui.cpp
  PUBLIC
    FILE_SET imgui_headers 
    TYPE HEADERS
    BASE_DIRS
      ext/imgui
    FILES
      ext/imgui/imgui.h
      ext/imgui/imconfig.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>  
)

if(IMGUI_PUGL_OPENGL3_BACKEND)

message(STATUS "imgui pugl opengl 3 backend")

target_sources(${PROJECT_NAME}
  PRIVATE
    ext/imgui/backends/imgui_impl_opengl3.cpp
    ext/imgui/backends/imgui_impl_pugl.cpp
  PUBLIC
    FILE_SET imgui_backends_headers
    TYPE HEADERS
    BASE_DIRS
      ext/imgui/backends
    FILES
      ext/imgui/backends/imgui_impl_opengl3.h
      ext/imgui/backends/imgui_impl_pugl.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends>
)

endif()

# implot
target_sources(${PROJECT_NAME}
  PRIVATE
    ext/implot/implot_demo.cpp
    ext/implot/implot_items.cpp
    ext/implot/implot.cpp
  PUBLIC
    FILE_SET implot_headers
    TYPE HEADERS
    BASE_DIRS
      ext/implot
    FILES
      ext/implot/implot.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/implot>
)

# imgui-knobs
target_sources(${PROJECT_NAME}
  PRIVATE
    ext/imgui-knobs/imgui-knobs.cpp
  PUBLIC
    FILE_SET imgui_knobs
    TYPE HEADERS
    BASE_DIRS
      ext/imgui-knobs
    FILES
      ext/imgui-knobs/imgui-knobs.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui-knobs>
)

# pugltk
target_sources(${PROJECT_NAME}
  PRIVATE
    src/Ui.cpp
    src/View.cpp
  PUBLIC
    FILE_SET HEADERS 
    TYPE HEADERS
    BASE_DIRS
      include
    FILES
      include/pugltk/Ui.h
      include/pugltk/View.h
)

# embedded fonts
if(${PUGLTK_EMBED_FONTS})

include(cmake/embed_ttf.cmake)

add_executable(imgui_ttf_font_binary_to_compressed_c
  ext/imgui/misc/fonts/binary_to_compressed_c.cpp
)

set(EMBEDDED_FONT_IN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts)
set(EMBEDDED_FONT_PATH ${CMAKE_CURRENT_BINARY_DIR}/include/pugltk/fonts)

file(MAKE_DIRECTORY ${EMBEDDED_FONT_PATH})

embed_ttf(${EMBEDDED_FONT_IN_PATH}/Cousine-Regular.ttf cousine_regular_ttf
  ${EMBEDDED_FONT_PATH}/cousine_regular_ttf.h
)

embed_ttf(${EMBEDDED_FONT_IN_PATH}/DroidSans.ttf droid_sans_ttf
  ${EMBEDDED_FONT_PATH}/droid_sans_ttf.h
)

embed_ttf(${EMBEDDED_FONT_IN_PATH}/Karla-Regular.ttf karla_regular_ttf
  ${EMBEDDED_FONT_PATH}/karla_regular_ttf.h
)

embed_ttf(${EMBEDDED_FONT_IN_PATH}/ProggyClean.ttf proggy_clean_ttf
  ${EMBEDDED_FONT_PATH}/proggy_clean_ttf.h
)

embed_ttf(${EMBEDDED_FONT_IN_PATH}/ProggyTiny.ttf proggy_tiny_ttf
  ${EMBEDDED_FONT_PATH}/proggy_tiny_ttf.h
)

embed_ttf(${EMBEDDED_FONT_IN_PATH}/Roboto-Medium.ttf roboto_medium_ttf
  ${EMBEDDED_FONT_PATH}/roboto_medium_ttf.h
)

target_compile_definitions(${PROJECT_NAME} 
  PUBLIC
    PUGLTK_EMBED_FONTS
)

target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET pugltk_fonts 
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_CURRENT_BINARY_DIR}/include
    FILES
      ${EMBEDDED_FONT_PATH}/cousine_regular_ttf.h
      ${EMBEDDED_FONT_PATH}/droid_sans_ttf.h
      ${EMBEDDED_FONT_PATH}/karla_regular_ttf.h
      ${EMBEDDED_FONT_PATH}/proggy_clean_ttf.h
      ${EMBEDDED_FONT_PATH}/proggy_tiny_ttf.h
      ${EMBEDDED_FONT_PATH}/roboto_medium_ttf.h
)
endif()

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED true
  POSITION_INDEPENDENT_CODE true
)

# install library
if(${install_pugltk})

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  FILE_SET glad_headers
  FILE_SET pugl_headers
  FILE_SET pugl_cpp_headers
  FILE_SET pugl_platform_headers
  FILE_SET imgui_headers
  FILE_SET imgui_backends_headers
  FILE_SET imgui_knobs
  FILE_SET pugltk_fonts
  FILE_SET implot_headers
  FILE_SET HEADERS
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install_cmake_config(${PROJECT_NAME})

endif()
