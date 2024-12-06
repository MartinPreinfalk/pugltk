cmake_minimum_required(VERSION 3.23)
project(pugl VERSION 1.0)

#include(../../cmake/install_cmake_config_macro.cmake)

if (UNIX AND NOT APPLE)
    set(PUGL_X11 ON)
    message(STATUS "platform unix was detected")
else()
    message(FATAL_ERROR "No supported platform was detected")
endif()

add_library(${PROJECT_NAME} STATIC)
add_library(ext::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    pugl/src/common.c
    pugl/src/internal.c
  PUBLIC
    FILE_SET HEADERS 
    TYPE HEADERS
    BASE_DIRS
      pugl/include
    FILES
      pugl/include/pugl/pugl.h
      pugl/include/pugl/attributes.h
  PUBLIC
    FILE_SET HEADERS 
    TYPE HEADERS
    BASE_DIRS
      pugl/bindings/cpp/include
    FILES
      pugl/bindings/cpp/include/pugl/pugl.hpp
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pugl/include> 
    #$<INSTALL_INTERFACE:pugl/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pugl/bindings/cpp/include> 
    #$<INSTALL_INTERFACE:pugl/bindings/cpp/include>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pugl/src> 
    #$<INSTALL_INTERFACE:pugl/src>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED true
  POSITION_INDEPENDENT_CODE true
)

if(PUGL_X11)

message(STATUS "pugl platform x11 and backend gl")

target_sources(${PROJECT_NAME}
  PRIVATE
    pugl/src/x11_gl.c
    pugl/src/x11.c
  PUBLIC
    FILE_SET HEADERS 
    TYPE HEADERS
    BASE_DIRS
      pugl/include
    FILES
      pugl/include/pugl/gl.h
)

find_package(X11 REQUIRED)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    $<BUILD_INTERFACE:${X11_X11_INCLUDE_PATH}> 
    #$<INSTALL_INTERFACE:${X11_X11_INCLUDE_PATH}>
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    X11
    GL
    Xrandr
    Xext
    Xcursor
)

endif()

install(TARGETS ${PROJECT_NAME} 
  EXPORT ${PROJECT_NAME}Targets 
  FILE_SET HEADERS
#  LIBRARY DESTINATION lib
#  ARCHIVE DESTINATION lib
#  RUNTIME DESTINATION bin
#  INCLUDES DESTINATION include
)

# include(GNUInstallDirs)
# install(DIRECTORY pugl/include/pugl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# install(DIRECTORY pugl/bindings/cpp/include/pugl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install_cmake_config(${PROJECT_NAME})