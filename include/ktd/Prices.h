#ifndef KTD_PRICES_H
#define KTD_PRICES_H

#include <cstdint>
#include "ktd/Tower.h"

namespace ktd {

int buildCost(TowerType type);
int upgradeCost(std::uint8_t currentLevel);

}

#endif  // KTD_PRICES_H
