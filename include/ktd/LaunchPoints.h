#ifndef KTD_LAUNCHPOINTS_H
#define KTD_LAUNCHPOINTS_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include "ktd/TowerType.h"

namespace ktd {

// Een plek op de toren waar een kogel zichtbaar vandaan komt, in pixels t.o.v. het MIDDEN VAN DE BOUWPLEK
// (de positie van de toren): x naar rechts, y naar BENEDEN, dus een negatieve y ligt boven dat midden.
struct LaunchPoint {
    double x;
    double y;
};

// ---- Archer Tower: de twee boogschutters ----
// Dit is dé plek om de startpositie van de pijlen aan te passen. Eén rij per toren-level (1, 2, 3), per rij twee
// schutters. De pijlen beginnen precies hier, en de groene stippen op de toren worden hier ook getekend,
// dus stip en pijl staan altijd op dezelfde plek. De schutters schieten om de beurt: eerst de eerste, dan de
// tweede, dan weer de eerste enzovoort.
inline constexpr std::array<std::array<LaunchPoint, 2>, 3> kArcherLaunchPoints = {{
    {{{-6.5, -41.3}, {6.5, -41.3}}},    // level 1
    {{{-1.4, -47.5}, {7.9, -47.5}}},    // level 2 (kokers en planken links en rechts van de opening)
    {{{0.7, -62.2}, {11.5, -62.2}}},    // level 3 (de vlaggenstok staat tussen de twee)
}};

// ---- Mage Tower: het magiërtje ----
// Eén punt per level (1, 2, 3): het midden van de bovenkant van de toren. De magiebol begint hier en de paarse
// stip op de toren wordt hier ook getekend.
inline constexpr std::array<std::array<LaunchPoint, 1>, 3> kMageLaunchPoints = {{
    {{{0.0, -41.4}}},     // level 1 (midden van de open top)
    {{{-1.6, -56.2}}},    // level 2 (midden van de bol)
    {{{-1.2, -70.7}}},    // level 3 (midden van de bol)
}};

// De lanceerpunten van een toren op dit level. Een lege lijst betekent: nog geen eigen punten, de kogels
// beginnen dan in het midden van de toren (zo is het bij de andere torens nu nog). Nieuwe torens voeg je
// hier toe met een eigen tabel zoals hierboven.
inline std::span<const LaunchPoint> launchPoints(TowerType type, std::uint8_t level)
{
    if (type == TowerType::Archer) {
        const std::size_t row = std::clamp<std::size_t>(level, 1, kArcherLaunchPoints.size()) - 1;
        return kArcherLaunchPoints[row];
    }
    if (type == TowerType::Mage) {
        const std::size_t row = std::clamp<std::size_t>(level, 1, kMageLaunchPoints.size()) - 1;
        return kMageLaunchPoints[row];
    }
    return {};
}

}

#endif  // KTD_LAUNCHPOINTS_H
