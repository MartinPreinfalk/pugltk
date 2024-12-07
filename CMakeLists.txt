cmake_minimum_required(VERSION 3.23)
project(pugltk VERSION 1.0)

# determine if pugltk is built as a subproject (using add_subdirectory) 
# or if it is the main project
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

option(build_examples "enables build of examples" OFF)
option(install_pugltk "enables build of examples" ${MAIN_PROJECT})
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(IMGUI_PUGL_OPENGL3_BACKEND "build with opengl3 backend" ON)
message(STATUS "IMGUI_PUGL_OPENGL3_BACKEND: ${IMGUI_PUGL_OPENGL3_BACKEND}")

# build the lib
add_subdirectory(pugltk)

# build the examples
if(${build_examples})
    add_subdirectory(examples)
endif()
