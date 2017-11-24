cmake_minimum_required(VERSION 2.8.12)

project(asdcplib)

# The version number.
file(STRINGS configure VERSION_STRING REGEX "PACKAGE_VERSION=.*")
STRING(REGEX REPLACE "(.*)([\n*[.]]+).*" "\\2" ${VERSION_STRING} "${${VERSION_STRING}}")
STRING(REGEX REPLACE "(PACKAGE_VERSION=')(.*)'" "\\2" VERSION_STRING ${VERSION_STRING})
STRING(REGEX REPLACE "([0-9]+)[.].*" "\\1" VERSION_MAJOR ${VERSION_STRING})
STRING(REGEX REPLACE ".*[.]([0-9]+)[.].*" "\\1" VERSION_MINOR ${VERSION_STRING})
STRING(REGEX REPLACE ".*[.]+([0-9]+)" "\\1" VERSION_PATCH ${VERSION_STRING})
message(STATUS "Version Number is ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Default Targets")
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  OFF)
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF) 
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

# Configure a header file to pass some of the CMake settings to the source code.
configure_file("${PROJECT_SOURCE_DIR}/src/info.in" "${PROJECT_BINARY_DIR}/src/info.h")

# Add the binary tree to the search path for include files so that we will find info.h.
include_directories("${PROJECT_BINARY_DIR}/src")
add_subdirectory(src)

set(CPACK_GENERATOR ZIP)
if(UNIX)
	set(CPACK_GENERATOR ${CPACK_GENERATOR};STGZ;TGZ)
endif(UNIX)
if(UNIX AND NOT APPLE)
	set(CPACK_GENERATOR ${CPACK_GENERATOR};DEB;RPM)
endif(UNIX AND NOT APPLE)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "asdcplib from cinecert.com")
set(CPACK_PACKAGE_VENDOR "CineCert LLC")
set(CPACK_PACKAGE_CONTACT "asdcplib@cinecert.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME}{VERSION_MAJOR}.${VERSION_MINOR})
include(CPack)
