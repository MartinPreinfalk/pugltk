#message("oat.cmake was included")
include(GNUInstallDirs)

macro (install_cmake_config target)
  # install cmake targets file
  install (EXPORT ${target}Targets
      FILE ${target}-targets.cmake
      NAMESPACE ${target}::
      DESTINATION lib/cmake/${target}
  )

  include(CMakePackageConfigHelpers)

  # Generate a package version file for this project
  write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cmake/${target}-config-version.cmake
    VERSION ${${target}_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Generate a config file for this project's exported targets
  configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/${target}-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/${target}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${target}
  )

  # Install the generated config files for this project's exported targets
  install(FILES
    ${CMAKE_BINARY_DIR}/cmake/${target}-config.cmake
    ${CMAKE_BINARY_DIR}/cmake/${target}-config-version.cmake
    DESTINATION lib/cmake/${target}
  )
endmacro()
