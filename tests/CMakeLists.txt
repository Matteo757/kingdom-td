add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE ktd_core Qt6::Test)
add_test(NAME test_core COMMAND test_core)

add_executable(test_player test_player.cpp)
target_link_libraries(test_player PRIVATE ktd_core Qt6::Test)
add_test(NAME test_player COMMAND test_player)

add_executable(test_towers test_towers.cpp)
target_link_libraries(test_towers PRIVATE ktd_core Qt6::Test)
add_test(NAME test_towers COMMAND test_towers)

add_executable(test_wave test_wave.cpp)
target_link_libraries(test_wave PRIVATE ktd_core Qt6::Test)
add_test(NAME test_wave COMMAND test_wave)

add_executable(test_game test_game.cpp)
target_link_libraries(test_game PRIVATE ktd_core Qt6::Test)
add_test(NAME test_game COMMAND test_game)

add_executable(test_enemies test_enemies.cpp)
target_link_libraries(test_enemies PRIVATE ktd_core Qt6::Test)
add_test(NAME test_enemies COMMAND test_enemies)

add_executable(test_build test_build.cpp)
target_link_libraries(test_build PRIVATE ktd_core Qt6::Test)
add_test(NAME test_build COMMAND test_build)

add_executable(test_spell test_spell.cpp)
target_link_libraries(test_spell PRIVATE ktd_core Qt6::Test)
add_test(NAME test_spell COMMAND test_spell)

add_executable(test_level test_level.cpp)
target_link_libraries(test_level PRIVATE ktd_core Qt6::Test)
add_test(NAME test_level COMMAND test_level)

add_executable(test_progression test_progression.cpp)
target_link_libraries(test_progression PRIVATE ktd_core Qt6::Test)
add_test(NAME test_progression COMMAND test_progression)

add_executable(test_levelloader test_levelloader.cpp)
target_link_libraries(test_levelloader PRIVATE ktd_core Qt6::Test)
add_custom_command(TARGET test_levelloader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/levels $<TARGET_FILE_DIR:test_levelloader>/levels
    VERBATIM)
add_test(NAME test_levelloader COMMAND test_levelloader)

add_executable(test_wavestart test_wavestart.cpp)
target_link_libraries(test_wavestart PRIVATE ktd_core Qt6::Test)
add_test(NAME test_wavestart COMMAND test_wavestart)

add_executable(test_projectile test_projectile.cpp)
target_link_libraries(test_projectile PRIVATE ktd_core Qt6::Test)
add_test(NAME test_projectile COMMAND test_projectile)

add_executable(test_slotmenu test_slotmenu.cpp)
target_link_libraries(test_slotmenu PRIVATE ktd_core Qt6::Test)
add_test(NAME test_slotmenu COMMAND test_slotmenu)

add_executable(test_routebounds test_routebounds.cpp)
target_link_libraries(test_routebounds PRIVATE ktd_core Qt6::Test)
add_test(NAME test_routebounds COMMAND test_routebounds)

add_executable(test_flag test_flag.cpp)
target_link_libraries(test_flag PRIVATE ktd_core Qt6::Test)
add_test(NAME test_flag COMMAND test_flag)

add_executable(test_sell test_sell.cpp)
target_link_libraries(test_sell PRIVATE ktd_core Qt6::Test)
add_test(NAME test_sell COMMAND test_sell)

add_executable(test_autostart test_autostart.cpp)
target_link_libraries(test_autostart PRIVATE ktd_core Qt6::Test)
add_test(NAME test_autostart COMMAND test_autostart)

add_executable(test_rangeselection test_rangeselection.cpp)
target_link_libraries(test_rangeselection PRIVATE ktd_core Qt6::Test)
add_test(NAME test_rangeselection COMMAND test_rangeselection)
