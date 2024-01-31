cmake_minimum_required(VERSION 3.0...3.28)

project(fastmod
  DESCRIPTION "A header file for fast 32-bit division remainders  on 64-bit hardware"
  LANGUAGES CXX
)

set (CMAKE_CXX_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
                message(STATUS "No build type selected, default to Release")
                set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()


add_library(fastmod INTERFACE)
enable_testing()
add_subdirectory(tests)
include(CMakePackageConfigHelpers)

set(FASTMOD_VERSION_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/fastmodConfigVersion.cmake")
set(FASTMOD_PROJECT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/module/fastmodConfig.cmake")
set(FASTMOD_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/fastmod")


configure_package_config_file("cmake/config.cmake.in"
                              "${FASTMOD_PROJECT_CONFIG}"
                              INSTALL_DESTINATION "${FASTMOD_CONFIG_INSTALL_DIR}")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES "${FASTMOD_PROJECT_CONFIG}" "${FASTMOD_VERSION_CONFIG}" DESTINATION "${FASTMOD_CONFIG_INSTALL_DIR}")
install(EXPORT ${PROJECT_NAME}-targets NAMESPACE fastmod:: DESTINATION "${FASTMOD_CONFIG_INSTALL_DIR}")

install(TARGETS fastmod
        EXPORT ${PROJECT_NAME}-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)



target_include_directories(fastmod INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCDIR}>)

add_library(fastmod::fastmod ALIAS fastmod)


set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Roaring bitmaps in C")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_RPM_PACKAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
include(CPack)