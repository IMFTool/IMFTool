cmake_minimum_required(VERSION 3.21.1)
cmake_policy(SET CMP0135 NEW)

set(EXE_NAME "IMF-Tool")
project(${EXE_NAME})

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

qt_standard_project_setup()

# Name of target executable

# Helps Eclipse finding include paths:
set_property(GLOBAL PROPERTY CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES ON)
# Get the SVN revision number if an svn command line client is
# available.  This version will run at build time rather than only
# at configuration generation time.
FIND_PROGRAM(SVN_EXECUTABLE svn
  DOC "subversion command line client")
FIND_PROGRAM(GIT_EXECUTABLE git
  DOC "git command line client")
set(SVN_REV 0)
set(VERSION_MAJOR 1)
set(VERSION_MINOR 9)
set(VERSION_PATCH 0)

# only do this if we have an svn client.
if (SVN_EXECUTABLE)
    MACRO(Subversion_GET_REVISION dir variable)
      EXECUTE_PROCESS(COMMAND ${SVN_EXECUTABLE} info ${dir}
        OUTPUT_VARIABLE ${variable}
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      STRING(REGEX REPLACE "^(.*\n)?Last Changed Rev: ([^\n]+).*"
        "\\2" ${variable} "${${variable}}")
    ENDMACRO(Subversion_GET_REVISION)

    Subversion_GET_REVISION(${CMAKE_SOURCE_DIR} SVN_REV)
    if (SVN_REV)
        set(VERSION_PATCH ${SVN_REV})
        message(STATUS "SVN Last Changed Revision Number is ${SVN_REV}")
    endif()
endif (SVN_EXECUTABLE)

if (NOT SVN_REV)
  if (GIT_EXECUTABLE)  
    MACRO(Git_GET_REVISION dir variable)
      EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE ${variable}
        OUTPUT_STRIP_TRAILING_WHITESPACE) 
      string(REPLACE "V" "" ${variable} "${${variable}}")
      string(REPLACE "." ";" ${variable} "${${variable}}")
    ENDMACRO(Git_GET_REVISION)
    Git_GET_REVISION(${CMAKE_SOURCE_DIR} GIT_REV)
  endif ()
  if (GIT_REV)
    list(GET GIT_REV 0 VERSION_MAJOR)
    list(GET GIT_REV 1 VERSION_MINOR)
    list(GET GIT_REV 2 VERSION_PATCH)
    message(STATUS "Git Last Changed Revision Number is " ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif (GIT_REV)
endif (NOT SVN_REV)
# The version number.
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
message(STATUS "version string is ${VERSION_STRING}")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Default Targets")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Generated Files")
set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER "Generated Files")

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
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Supports browsing and editing of IMF packages.")
set(CPACK_PACKAGE_VENDOR "HSRM")
set(CPACK_PACKAGE_CONTACT "imftool@t-online.de")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME}{CTL_MAJOR_VERSION}.${CTL_MINOR_VERSION})
include(CPack)
file(COPY "${PROJECT_SOURCE_DIR}/regxmllib" DESTINATION "${PROJECT_BINARY_DIR}/src")
file(COPY "${PROJECT_SOURCE_DIR}/photon" DESTINATION "${PROJECT_BINARY_DIR}/src")
file(COPY "${PROJECT_SOURCE_DIR}/files" DESTINATION "${PROJECT_BINARY_DIR}/src")
