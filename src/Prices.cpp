#include "ktd/Prices.h"

#include <map>

namespace ktd {

int buildCost(TowerType type)
{
    static const std::map<TowerType, int> costs = {
        {TowerType::Archer, 70},
        {TowerType::Cannon, 120},
        {TowerType::Mage, 100},
        {TowerType::Barracks, 80},
        {TowerType::Artillery, 150},
    };
    return costs.at(type);
}

int upgradeCost(std::uint8_t currentLevel)
{
    static const std::map<std::uint8_t, int> costs = {
        {1, 90},
        {2, 140},
    };
    return costs.at(currentLevel);
}

}