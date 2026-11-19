#include "ktd/EnemyFactory.h"

#include <stdexcept>
#include "ktd/DarkMage.h"
#include "ktd/Goblin.h"
#include "ktd/Orc.h"
#include "ktd/Skeleton.h"

namespace ktd {

std::unique_ptr<Enemy> makeEnemy(EnemyType type)
{
    switch (type) {
    case EnemyType::Goblin:
        return std::make_unique<Goblin>();
    case EnemyType::Skeleton:
        return std::make_unique<Skeleton>();
    case EnemyType::Orc:
        return std::make_unique<Orc>();
    case EnemyType::DarkMage:
        return std::make_unique<DarkMage>();
    }
    throw std::invalid_argument("Unknown enemy type");
}

}