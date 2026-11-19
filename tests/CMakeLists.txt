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
