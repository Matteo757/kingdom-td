#ifndef KTD_WAVEMANAGER_H
#define KTD_WAVEMANAGER_H

#include <cstddef>
#include <vector>
#include "ktd/EnemyFactory.h"

namespace ktd {

// Een wave is een geordende lijst vijanden die met een vast interval verschijnen.
struct Wave {
    std::vector<EnemyType> enemies;
    double spawnInterval = 1.0;
};

// Hoe een volgende wave begint.
//  Automatic: vanzelf, na de wachttijd (gedrag van voor de Start Wave-knop, o.a. in de tests).
//  Manual:    pas als de speler erom vraagt met startNextWave(). Waves mogen elkaar dan overlappen:
//             vijanden van eerdere waves blijven gewoon in Game bestaan.
enum class StartMode { Automatic, Manual };

// Vroeg-startbonus: 3 goud per hele seconde die nog over is van de wachttijd, met een maximum van 60.
constexpr int kEarlyBonusPerSecond = 3;
constexpr int kMaxEarlyBonus = 60;

// Zuivere logica (geen Qt): bepaalt aan de hand van verstreken tijd welke vijanden
// er verschijnen. Daardoor is de wavetiming volledig te testen zonder GUI.
class WaveManager {
public:
    // waveDelay is de tijd tussen de laatste vijand van een wave en de volgende wave. In Manual-modus is dat
    // de bonustimer (20 seconden, zie kWaveDelay in WaveFactory.h); startDelay wordt daar niet gebruikt.
    explicit WaveManager(std::vector<Wave> waves, double startDelay = 3.0, double waveDelay = 6.0,
                         StartMode mode = StartMode::Automatic);

    // Geeft de vijanden terug die tijdens dit tijdsstapje moeten verschijnen.
    std::vector<EnemyType> update(double dt);

    bool finished() const;
    std::size_t currentWave() const;   // aantal waves waarvan al een vijand verschenen is
    std::size_t waveCount() const;

    // --- Handmatig starten (alleen StartMode::Manual) ---
    // Een nieuwe wave kan starten als er geen wave meer aan het verschijnen is en er nog waves over zijn.
    // Dat geldt ook voor de eerste wave, en ook als er nog vijanden van eerdere waves leven.
    bool canStartNextWave() const;
    // Start de volgende wave meteen en geeft het bonusgoud terug (0 bij de eerste wave of als de timer op is).
    int startNextWave();
    // Bonus als de speler nu zou starten: floor(resterende seconden) * 3, begrensd op 0..60.
    int earlyBonus() const;
    // Resterende seconden van de bonustimer (loopt vanaf de laatste vijand van de vorige wave).
    double timeUntilNextWave() const;
    // True zolang er nog geen enkele wave gestart is (knop heet dan "Start Wave" i.p.v. "Next Wave").
    bool firstWavePending() const;
    StartMode startMode() const;

private:
    std::vector<Wave> m_waves;
    double m_waveDelay;
    double m_countdown;
    StartMode m_mode;
    bool m_running = false;          // Manual: er is een wave gestart die nog vijanden moet laten verschijnen
    std::size_t m_waveIndex = 0;
    std::size_t m_spawned = 0;       // aantal gespawnde vijanden in de huidige wave
    std::size_t m_wavesStarted = 0;
};

}

#endif  // KTD_WAVEMANAGER_H
