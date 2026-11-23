// Vereiste 13: 13. correct protections (for header files)
// Include guards (#ifndef/#define/#endif) voorkomen dat deze header twee keer in dezelfde vertaaleenheid terechtkomt.
#ifndef KTD_GAME_H
#define KTD_GAME_H

#include <cstddef>
#include <memory>
#include <vector>
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/EnemyFactory.h"
#include "ktd/Path.h"
#include "ktd/Player.h"
#include "ktd/Prices.h"
#include "ktd/Tower.h"
#include "ktd/TowerFactory.h"
#include "ktd/Vec2.h"
#include "ktd/WaveManager.h"
#include "ktd/Fireball.h"
#include "ktd/LevelData.h"

namespace ktd {

struct Shot {
    Vec2 from;
    Vec2 to;
    DamageType type;
    TowerType tower;         // bepaalt welke kogel-sprite getoond wordt
    std::uint8_t level;      // torenlevel = stage (1..3) van de sprite
    const Enemy* target;     // alleen als sleutel om de vijand op het scherm te volgen, nooit dereferencen
};

struct SpawnEvent {
    const Enemy* enemy;
    EnemyType type;
};

struct FrameEvents {
    std::vector<SpawnEvent> spawned;
    std::vector<Shot> shots;
    std::vector<const Enemy*> removed;
    int goldEarned = 0;
    int livesLost = 0;
};

struct BuildSlot {
    Vec2 position;
    Tower* tower = nullptr;
};

// Vereiste 6: useful and correct object composition (explain why)
// Game bevat zelf een Path, Player, WaveManager, de bouwplekken en de torens en vijanden (has-a): die delen
// zijn nergens anders nodig en leven precies zo lang als het spel. Hierdoor bevat de GUI geen
// spelregels meer en kan de volledige spelloop zonder Qt getest worden.
class Game {
public:
    Game(Path path, Player player, WaveManager waves, std::vector<Vec2> slotPositions = {});

    static Game defaultLevel();
    static Game level(int number);
    static Game fromLevel(const LevelData& data);   // bouwt een spel uit gegevens van een levelbestand

    void addTower(std::unique_ptr<Tower> tower);
    bool canBuild(std::size_t slot, TowerType type) const;
    bool buildTower(std::size_t slot, TowerType type);
    bool canUpgrade(std::size_t slot) const;
    bool upgradeTower(std::size_t slot);
    bool castFireball(const Vec2& target);
    bool fireballReady() const;
    FrameEvents update(double dt);

    const Path& path() const;
    const Player& player() const;
    const WaveManager& waves() const;
    const std::vector<std::unique_ptr<Tower>>& towers() const;
    const std::vector<BuildSlot>& buildSlots() const;
    const std::vector<std::unique_ptr<Enemy>>& enemies() const;

    bool isGameOver() const;
    bool isWon() const;

private:
    void removeFinishedEnemies(FrameEvents& events);

    Path m_path;
    Player m_player;
    WaveManager m_waves;
    std::vector<BuildSlot> m_slots;
    std::vector<std::unique_ptr<Tower>> m_towers;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::unique_ptr<Spell> m_fireball = std::make_unique<Fireball>();
};

}

#endif  // KTD_GAME_H
