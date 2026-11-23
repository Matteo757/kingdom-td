#include "ktd/WaveFactory.h"

#include <algorithm>
#include <stdexcept>

namespace ktd {

Wave buildWave(int number)
{
    if (number < 1) {
        throw std::invalid_argument("Wave number must be at least 1");
    }

    int skeletons = 2 + number;
    int goblins = 1 + number;
    int orcs = number >= 3 ? (number - 1) / 2 : 0;
    int darkMages = number >= 5 ? (number - 3) / 2 : 0;

    Wave wave;
    wave.spawnInterval = std::max(0.7, 1.4 - 0.07 * (number - 1));
    while (skeletons > 0 || goblins > 0 || orcs > 0 || darkMages > 0) {
        if (skeletons-- > 0) wave.enemies.push_back(EnemyType::Skeleton);
        if (goblins-- > 0) wave.enemies.push_back(EnemyType::Goblin);
        if (orcs-- > 0) wave.enemies.push_back(EnemyType::Orc);
        if (darkMages-- > 0) wave.enemies.push_back(EnemyType::DarkMage);
    }
    return wave;
}

std::vector<Wave> buildWaves(int count)
{
    std::vector<Wave> waves;
    for (int number = 1; number <= count; ++number) {
        waves.push_back(buildWave(number));
    }
    return waves;
}

}