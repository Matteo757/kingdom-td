#pragma once

#include <memory>
#include <vector>
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/EnemyFactory.h"
#include "ktd/Path.h"
#include "ktd/Player.h"
#include "ktd/Tower.h"
#include "ktd/Vec2.h"
#include "ktd/WaveManager.h"

namespace ktd {

struct Shot {
    Vec2 from;
    Vec2 to;
    DamageType type;
};

struct SpawnEvent {
    const Enemy* enemy;
    EnemyType type;
};

// Wat er in één tick gebeurd is. De GUI leest dit uit om te tekenen en signals uit te sturen,
// zodat Game zelf niets van Qt hoeft te weten.
struct FrameEvents {
    std::vector<SpawnEvent> spawned;
    std::vector<Shot> shots;
    std::vector<const Enemy*> removed;   // enkel als sleutel te gebruiken, het object bestaat niet meer
    int goldEarned = 0;
    int livesLost = 0;
};

// Vereiste 6: useful and correct object composition (explain why)
// Game bevat zelf een Path, Player, WaveManager en de torens en vijanden (has-a): die delen
// zijn nergens anders nodig en leven precies zo lang als het spel. Hierdoor bevat de GUI geen
// spelregels meer en kan de volledige spelloop zonder Qt getest worden.
class Game {
public:
    Game(Path path, Player player, WaveManager waves);

    static Game defaultLevel();

    void addTower(std::unique_ptr<Tower> tower);
    FrameEvents update(double dt);

    const Path& path() const;
    const Player& player() const;
    const WaveManager& waves() const;
    const std::vector<std::unique_ptr<Tower>>& towers() const;
    const std::vector<std::unique_ptr<Enemy>>& enemies() const;

    bool isGameOver() const;
    bool isWon() const;

private:
    void removeFinishedEnemies(FrameEvents& events);

    Path m_path;
    Player m_player;
    WaveManager m_waves;
    std::vector<std::unique_ptr<Tower>> m_towers;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
};

}