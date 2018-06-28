cmake_minimum_required(VERSION 3.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets REQUIRED)

add_executable(sbdemo "sbuffer.cpp" "sbdemo.qrc")

target_link_libraries(sbdemo Qt5::Widgets)

unset(QT_QMAKE_EXECUTABLE)
