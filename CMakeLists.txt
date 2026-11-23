cmake_minimum_required(VERSION 3.21)
project(kingdom-td LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

add_library(ktd_core STATIC
    src/Path.cpp
    src/Enemy.cpp
    src/Tower.cpp
    src/Projectile.cpp
    src/Soldier.cpp
    src/Prices.cpp
    src/TowerFactory.cpp
    src/BarracksTower.cpp
    src/Player.cpp
    src/EnemyFactory.cpp
    src/WaveManager.cpp
    src/Game.cpp
    src/WaveFactory.cpp
    src/LevelLoader.cpp
    src/SlotMenu.cpp
)
find_package(Threads REQUIRED)

target_include_directories(ktd_core PUBLIC include)
target_include_directories(ktd_core PRIVATE third_party)   # nlohmann/json (single header, MIT)
target_link_libraries(ktd_core PUBLIC Threads::Threads)

add_executable(kingdom-td
    src/main.cpp
    resources.qrc
    src/MainWindow.cpp
    src/GameView.cpp
    src/Hud.cpp
    include/ktd/MainWindow.h
    include/ktd/GameView.h
    include/ktd/Hud.h
)
target_link_libraries(kingdom-td PRIVATE ktd_core Qt6::Widgets)

# De levelbestanden (JSON) worden naast het programma gezet; het programma leest ze bij het starten.
add_custom_command(TARGET kingdom-td POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/levels $<TARGET_FILE_DIR:kingdom-td>/levels
    VERBATIM)

enable_testing()
add_subdirectory(tests)