cmake_minimum_required(VERSION 3.21.1)
project(QtAppBase)
include(QtAppBase)

qt_app_project_setup(PROJECT_SETUP_INFO)
project(${PROJECT_SETUP_INFO})

qt_app_setup()

add_subdirectory(src)
