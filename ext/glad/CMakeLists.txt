cmake_minimum_required(VERSION 3.23)
project(glad VERSION 1.0)

add_library(${PROJECT_NAME} STATIC)
add_library(ext::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    glad/src/glad/glad.c
  PUBLIC
    FILE_SET HEADERS 
    TYPE HEADERS
    BASE_DIRS
      glad/include
    FILES
      glad/include/glad/glad.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>  
#    $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED true
  POSITION_INDEPENDENT_CODE true
)

install(TARGETS ${PROJECT_NAME} 
  EXPORT ${PROJECT_NAME}Targets 
  FILE_SET HEADERS
#  LIBRARY DESTINATION lib
#  ARCHIVE DESTINATION lib
#  RUNTIME DESTINATION bin
#  INCLUDES DESTINATION include
)

#include(../../cmake/install_cmake_config_macro.cmake)
install_cmake_config(${PROJECT_NAME})