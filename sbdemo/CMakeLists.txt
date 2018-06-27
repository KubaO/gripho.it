cmake_minimum_required(VERSION 3.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets)

add_executable(sbdemo "sbuffer.cpp")

target_link_libraries(sbdemo Qt5::Widgets)

unset(QT_QMAKE_EXECUTABLE)
