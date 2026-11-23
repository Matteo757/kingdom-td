#ifndef KTD_LEVELDATA_H
#define KTD_LEVELDATA_H

#include <vector>
#include "ktd/Vec2.h"

namespace ktd {

// De gegevens van één level zoals ze in een JSON-bestand staan.
// Alle coordinaten staan in sceneruimte (1024x576), net als de kaartafbeelding.
struct LevelData {
    int number = 0;            // nummer van het level (bepaalt welke kaart getoond wordt)
    int startGold = 0;
    int startLives = 0;
    int waveCount = 0;
    std::vector<Vec2> route;   // waypoints van de weg, van spawn naar kasteel
    std::vector<Vec2> slotPositions;   // midden van elke bouwplek
};

}  // namespace ktd

#endif  // KTD_LEVELDATA_H
