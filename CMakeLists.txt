cmake_minimum_required(VERSION 3.16)

project(fastmod
  DESCRIPTION "A header file for fast 32-bit division and remainders on 64-bit hardware"
  LANGUAGES C CXX
  VERSION 0.1.0
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

add_library(fastmod INTERFACE)
add_library(fastmod::fastmod ALIAS fastmod)

target_include_directories(fastmod INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(FASTMOD_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/fastmod")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/fastmodConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
  "cmake/config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/fastmodConfig.cmake"
  INSTALL_DESTINATION "${FASTMOD_CONFIG_INSTALL_DIR}"
)

install(TARGETS fastmod EXPORT ${PROJECT_NAME}-targets)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/fastmodConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/fastmodConfigVersion.cmake"
  DESTINATION "${FASTMOD_CONFIG_INSTALL_DIR}"
)
install(EXPORT ${PROJECT_NAME}-targets
  NAMESPACE fastmod::
  DESTINATION "${FASTMOD_CONFIG_INSTALL_DIR}"
)

set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A header file for fast 32-bit division and remainders on 64-bit hardware.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RPM_PACKAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
include(CPack)
