cmake_minimum_required(VERSION 3.16)
project(eq_test VERSION 1.0)

include(GNUInstallDirs)

# determine if eq_test is built as a subproject (using add_subdirectory) 
# or if it is the main project
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

if(MAIN_PROJECT)
find_package(pugltk REQUIRED)
endif()

add_executable(${PROJECT_NAME}
  src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    pugltk::pugltk
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED true
)

install(TARGETS ${target} EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
)
