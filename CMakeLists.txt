cmake_minimum_required(VERSION 3.23)
project(pugltk VERSION 1.0)

include(cmake/install_cmake_config_macro.cmake)

# determine if pugltk is built as a subproject (using add_subdirectory) 
# or if it is the main project
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()


option(build_examples "enables build of examples" OFF)
option(install_pugltk "enables build of examples" ${MAIN_PROJECT})
option(BUILD_SHARED_LIBS "Build using shared libraries" ${MAIN_PROJECT})

# # blacklist for targets from subprojects
# # targets that are expected to exist as subprojects
# # are blacklisted for the find_package macro here
# set(as_subproject
#   glad
#   pugl
#   imgui
#   pugltk
# )

# # This only works if find_package is redefined once
# macro(find_package)
#   if(NOT "${ARGV0}" IN_LIST as_subproject)
#     _find_package(${ARGV})
#   endif()
# endmacro()

# build the external dependencies
add_subdirectory(ext)

# build the lib
add_subdirectory(pugltk)

# build the examples
if(${build_examples})
    add_subdirectory(examples)
endif()
