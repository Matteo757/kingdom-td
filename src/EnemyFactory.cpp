#include "ktd/EnemyFactory.h"

#include <stdexcept>
#include "ktd/Grunt.h"
#include "ktd/Runner.h"

namespace ktd {

std::unique_ptr<Enemy> makeEnemy(EnemyType type)
{
    switch (type) {
    case EnemyType::Grunt:
        return std::make_unique<Grunt>();
    case EnemyType::Runner:
        return std::make_unique<Runner>();
    }
    throw std::invalid_argument("Unknown enemy type");
}

}