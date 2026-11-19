#pragma once

#include <cstdint>
#include "ktd/Tower.h"

namespace ktd {

int buildCost(TowerType type);
int upgradeCost(std::uint8_t currentLevel);

}