cmake_minimum_required(VERSION 3.23)
project(imgui VERSION 1.0)

option(IMGUI_PUGL_OPENGL3_BACKEND "build with opengl3 backend" ON)
message(STATUS "IMGUI_PUGL_OPENGL3_BACKEND: ${IMGUI_PUGL_OPENGL3_BACKEND}")

add_library(${PROJECT_NAME} STATIC)
add_library(ext::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui.cpp
  PUBLIC
    FILE_SET HEADERS 
    TYPE HEADERS
    BASE_DIRS
      imgui
    FILES
      imgui/imgui.h
      imgui/imconfig.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>  
    #$<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED true
  POSITION_INDEPENDENT_CODE true
)

#install(DIRECTORY imgui DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


if(IMGUI_PUGL_OPENGL3_BACKEND)

message(STATUS "imgui pugl opengl 3 backend")

#find_package(pugl REQUIRED)

target_sources(${PROJECT_NAME}
  PRIVATE
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_pugl.cpp
  PUBLIC
    FILE_SET imgui_backends_headers
    TYPE HEADERS
    BASE_DIRS
      imgui/backends
    FILES
      imgui/backends/imgui_impl_opengl3.h
      imgui/backends/imgui_impl_pugl.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends>  
    #$<INSTALL_INTERFACE:imgui/backends>
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    GL
    ext::pugl
)

endif()

install(TARGETS ${PROJECT_NAME} 
  EXPORT ${PROJECT_NAME}Targets 
  FILE_SET HEADERS
  FILE_SET imgui_backends_headers
#  LIBRARY DESTINATION lib
#  ARCHIVE DESTINATION lib
#  RUNTIME DESTINATION bin
#  INCLUDES DESTINATION include
)

install_cmake_config(${PROJECT_NAME})