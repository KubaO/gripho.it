cmake_minimum_required(VERSION 3.1)
project(v6502)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets REQUIRED)

add_executable(try "try.cpp")

unset(QT_QMAKE_EXECUTABLE)
