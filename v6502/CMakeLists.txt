cmake_minimum_required(VERSION 3.1)
project(v6502)

set(CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets REQUIRED)

add_executable(try "try.cpp" "asm_6502.cpp" "conio.cpp")
target_link_libraries(try Qt5::Widgets)

unset(QT_QMAKE_EXECUTABLE)
