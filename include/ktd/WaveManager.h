#pragma once

#include <cstddef>
#include <vector>
#include "ktd/EnemyFactory.h"

namespace ktd {

// Een wave is een geordende lijst vijanden die met een vast interval verschijnen.
struct Wave {
    std::vector<EnemyType> enemies;
    double spawnInterval = 1.0;
};

// Zuivere logica (geen Qt): bepaalt aan de hand van verstreken tijd welke vijanden
// er verschijnen. Daardoor is de wavetiming volledig te testen zonder GUI.
class WaveManager {
public:
    explicit WaveManager(std::vector<Wave> waves, double startDelay = 3.0, double waveDelay = 6.0);

    // Geeft de vijanden terug die tijdens dit tijdsstapje moeten verschijnen.
    std::vector<EnemyType> update(double dt);

    bool finished() const;
    std::size_t currentWave() const;   // aantal waves waarvan al een vijand verschenen is
    std::size_t waveCount() const;

private:
    std::vector<Wave> m_waves;
    double m_waveDelay;
    double m_countdown;
    std::size_t m_waveIndex = 0;
    std::size_t m_spawned = 0;       // aantal gespawnde vijanden in de huidige wave
    std::size_t m_wavesStarted = 0;
};

}