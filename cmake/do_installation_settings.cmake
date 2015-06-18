set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Install the sources.
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Eigen 3
  do_message("Installing Eigen")
  install(DIRECTORY ${DO_Sara_ThirdParty_DIR}/eigen/Eigen
          DESTINATION include
          COMPONENT Eigen3)
  set(CPACK_COMPONENT_Eigen3_REQUIRED 1)

  # DO-${DO_PROJECT_NAME} source files
  install(FILES ${DO_${DO_PROJECT_NAME}_DIR}/COPYING.README
                ${DO_${DO_PROJECT_NAME}_DIR}/COPYING.MPL2
          DESTINATION include/DO/${DO_PROJECT_NAME}
          COMPONENT Sources)
  install(DIRECTORY ${DO_${DO_PROJECT_NAME}_DIR}/cmake
          DESTINATION include/DO/${DO_PROJECT_NAME}
          COMPONENT Sources)
  install(DIRECTORY ${DO_${DO_PROJECT_NAME}_DIR}/src/DO
          DESTINATION include/
          COMPONENT Sources)
  install(FILES ${DO_${DO_PROJECT_NAME}_SOURCE_DIR}/Defines.hpp
          DESTINATION include/DO/${DO_PROJECT_NAME}
          COMPONENT Sources)
  set(CPACK_COMPONENT_Sources_REQUIRED 1)
endif ()

# DO-${DO_PROJECT_NAME} component libraries
foreach (component ${DO_COMPONENTS})
  do_message("Installing DO.${component}")
  include(${DO_${component}_USE_FILE})
endforeach (component)


# List all available components for installation.
set(CPACK_COMPONENTS_ALL Eigen3 Sources Core Graphics Libraries)


set(CPACK_PACKAGE_NAME "libdo-sara")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dbg")
endif ()

set(CPACK_PACKAGE_VENDOR "DO-CV")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "DO-CV - An easy-to-use C++ set of libraries for computer vision")
set(CPACK_RESOURCE_FILE_LICENSE "${DO_${DO_PROJECT_NAME}_DIR}/COPYING.README")
set(CPACK_PACKAGE_CONTACT "David OK")

set(CPACK_PACKAGE_VERSION_MAJOR ${DO_${DO_PROJECT_NAME}_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DO_${DO_PROJECT_NAME}_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DO_${DO_PROJECT_NAME}_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${DO_${DO_PROJECT_NAME}_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DO/${DO_PROJECT_NAME}")


# Specific variables for Debian packages.
set(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "cmake, libjpeg-dev, libpng-dev, libtiff5-dev, qtbase5-dev")

if (WIN32)
  set(CPACK_GENERATOR NSIS)
elseif (UNIX)
  set(CPACK_GENERATOR "DEB")
endif()