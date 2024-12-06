cmake_minimum_required(VERSION 3.16)
project(hello_pugltk VERSION 1.0)

include(GNUInstallDirs)

find_package(pugltk REQUIRED)

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
