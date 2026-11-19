#include "ktd/WaveManager.h"

#include <utility>

namespace ktd {

WaveManager::WaveManager(std::vector<Wave> waves, double startDelay, double waveDelay)
    : m_waves(std::move(waves))
    , m_waveDelay(waveDelay)
    , m_countdown(startDelay)
{
}

std::vector<EnemyType> WaveManager::update(double dt)
{
    std::vector<EnemyType> spawned;
    if (finished()) {
        return spawned;
    }

    m_countdown -= dt;
    // while i.p.v. if: bij een grote dt kunnen er meerdere vijanden in één stap verschijnen.
    while (m_countdown <= 0.0 && !finished()) {
        const Wave& wave = m_waves[m_waveIndex];
        if (wave.enemies.empty()) {
            ++m_waveIndex;
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

}