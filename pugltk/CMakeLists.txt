cmake_minimum_required(VERSION 3.23)
project(pugltk VERSION 1.0)

include(GNUInstallDirs)

#find_package(glad REQUIRED)
#find_package(pugl REQUIRED)
#find_package(imgui REQUIRED)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

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


target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    ext::glad
    ext::pugl
    ext::imgui
)

# target_link_libraries(${PROJECT_NAME}
#   PUBLIC
#     "$<BUILD_INTERFACE:ext::glad>"
#     "$<BUILD_INTERFACE:ext::pugl>"
#     "$<BUILD_INTERFACE:ext::imgui>"
# )

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED true
)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  FILE_SET HEADERS
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
#  RUNTIME DESTINATION bin
#  INCLUDES DESTINATION include
)

#include(GNUInstallDirs)
#install(DIRECTORY include/pugltk DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install_cmake_config(${PROJECT_NAME})
