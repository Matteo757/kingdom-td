#pragma once

#include <cstdint>
#include <memory>
#include "ktd/Enemy.h"

namespace ktd {

enum class EnemyType : std::uint8_t { Grunt, Runner };

// Maakt een vijand aan op basis van zijn type. Zo hoeven WaveManager en Game
// de concrete klassen niet te kennen: een wave is gewoon een lijst van EnemyType.
std::unique_ptr<Enemy> makeEnemy(EnemyType type);

}