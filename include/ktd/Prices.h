#ifndef KTD_PRICES_H
#define KTD_PRICES_H

#include <cstdint>
#include "ktd/Tower.h"

namespace ktd {

int buildCost(TowerType type);
int upgradeCost(std::uint8_t currentLevel);

// Percentage van het totaal bestede goud (bouwen + alle upgrades) dat de speler terugkrijgt bij het verkopen.
constexpr int kSellPercent = 60;

// Wat een gebouw oplevert als het verkocht wordt: floor(investering x 60%). Het rekent met gehele getallen
// (investering * 60 / 100), want 150 * 0.6 in een double geeft 90.00000000000001 en zulke afrondfouten
// kunnen een euro schelen.
int sellRefund(int totalInvested);

}

#endif  // KTD_PRICES_H
