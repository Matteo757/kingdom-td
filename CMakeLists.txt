cmake_minimum_required(VERSION 3.21)
project(kingdom-td LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

add_library(ktd_core STATIC
    src/Path.cpp
    src/Enemy.cpp
)
target_include_directories(ktd_core PUBLIC include)

add_executable(kingdom-td
    src/main.cpp
    src/MainWindow.cpp
    src/GameView.cpp
    include/ktd/MainWindow.h
    include/ktd/GameView.h
)
target_link_libraries(kingdom-td PRIVATE ktd_core Qt6::Widgets)

enable_testing()
add_subdirectory(tests)