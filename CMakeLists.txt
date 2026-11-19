cmake_minimum_required(VERSION 3.21)
project(KingdomTD VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(ktd_core
    include/ktd/MainWindow.h
    src/MainWindow.cpp
)
target_include_directories(ktd_core PUBLIC include)
target_link_libraries(ktd_core PUBLIC Qt6::Widgets)

add_executable(kingdom_td src/main.cpp)
target_link_libraries(kingdom_td PRIVATE ktd_core)