#include "ktd/WaveManager.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ktd {

WaveManager::WaveManager(std::vector<Wave> waves, double startDelay, double waveDelay, StartMode mode)
    : m_waves(std::move(waves))
    , m_waveDelay(waveDelay)
    , m_countdown(startDelay)
    , m_mode(mode)
{
}

std::vector<EnemyType> WaveManager::update(double dt)
{
    std::vector<EnemyType> spawned;
    if (finished()) {
        return spawned;
    }

    bool startedAutomatically = false;
    if (m_mode == StartMode::Manual && !m_running) {
        // Wachten op de speler. Na de laatste vijand van een wave telt de bonustimer af naar 0.
        m_countdown = std::max(0.0, m_countdown - dt);
        if (!autoStartDue()) {
            return spawned;
        }
        // De timer is op en de speler heeft niet geklikt: de volgende wave begint nu vanzelf. Er wordt hier geen
        // bonus uitbetaald, want alleen startNextWave() geeft een bonus terug.
        beginWave();
        startedAutomatically = true;
    }

    // Bij een automatische start is dt al gebruikt om de timer naar 0 te brengen; niet nog een keer aftrekken,
    // anders zouden bij een grote dt meteen meerdere vijanden van de nieuwe wave verschijnen.
    if (!startedAutomatically) {
        m_countdown -= dt;
    }
    // while i.p.v. if: bij een grote dt kunnen er meerdere vijanden in één stap verschijnen.
    while (m_countdown <= 0.0 && !finished()) {
        const Wave& wave = m_waves[m_waveIndex];
        if (wave.enemies.empty()) {
            ++m_waveIndex;
            if (m_mode == StartMode::Manual) {
                m_running = false;
                m_countdown = m_waveDelay;
                break;
            }
            continue;
        }
        if (m_spawned == 0) {
            ++m_wavesStarted;
        }
        spawned.push_back(wave.enemies[m_spawned]);
        ++m_spawned;
        if (m_spawned >= wave.enemies.size()) {
            ++m_waveIndex;
            m_spawned = 0;
            if (m_mode == StartMode::Manual) {
                // Laatste vijand is verschenen: de bonustimer start nu, precies op waveDelay (20 s).
                m_running = false;
                m_countdown = m_waveDelay;
                break;
            }
            m_countdown += m_waveDelay;
        } else {
            m_countdown += wave.spawnInterval;
        }
    }
    return spawned;
}

bool WaveManager::finished() const
{
    return m_waveIndex >= m_waves.size();
}

std::size_t WaveManager::currentWave() const
{
    return m_wavesStarted;
}

std::size_t WaveManager::waveCount() const
{
    return m_waves.size();
}

bool WaveManager::canStartNextWave() const
{
    return m_mode == StartMode::Manual && !m_running && !finished();
}

int WaveManager::startNextWave()
{
    if (!canStartNextWave()) {
        return 0;
    }
    const int bonus = earlyBonus();   // berekend op het moment van klikken, vóór de timer herstart
    beginWave();
    return bonus;
}

void WaveManager::beginWave()
{
    m_running = true;
    m_countdown = 0.0;                // de eerste vijand verschijnt bij de eerstvolgende update()
}

bool WaveManager::autoStartDue() const
{
    // m_waveIndex is 0 zolang de eerste wave nog niet gestart is: daar is geen timer en dus ook geen automatische start.
    return canStartNextWave() && m_waveIndex > 0 && m_countdown <= 0.0;
}

int WaveManager::earlyBonus() const
{
    // Voor de eerste wave is er geen vorige wave en dus geen bonus.
    if (!canStartNextWave() || m_waveIndex == 0) {
        return 0;
    }
    const int secondsLeft = static_cast<int>(std::floor(m_countdown));
    return std::clamp(secondsLeft * kEarlyBonusPerSecond, 0, kMaxEarlyBonus);
}

double WaveManager::timeUntilNextWave() const
{
    return std::max(0.0, m_countdown);
}

bool WaveManager::firstWavePending() const
{
    return m_waveIndex == 0 && !m_running;
}

StartMode WaveManager::startMode() const
{
    return m_mode;
}

}