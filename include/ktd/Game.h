// Vereiste 13: 13. correct protections (for header files)
// Include guards (#ifndef/#define/#endif) voorkomen dat deze header twee keer in dezelfde vertaaleenheid terechtkomt.
#ifndef KTD_GAME_H
#define KTD_GAME_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/EnemyFactory.h"
#include "ktd/Path.h"
#include "ktd/Player.h"
#include "ktd/Projectile.h"
#include "ktd/RouteBounds.h"
#include "ktd/Prices.h"
#include "ktd/Tower.h"
#include "ktd/TowerFactory.h"
#include "ktd/Vec2.h"
#include "ktd/WaveManager.h"
#include "ktd/Fireball.h"
#include "ktd/LevelData.h"

namespace ktd {

// Afmetingen van de kaart in sceneruimte (de kaartafbeelding wordt op deze grootte getekend).
constexpr double kMapWidth = 1024.0;
constexpr double kMapHeight = 576.0;
// Een vlag staat minstens zo ver (px) van het midden van een bouwplek: de bouwplaats zelf is geen weg.
constexpr double kPadClearance = 30.0;
// Een vlag staat minstens zo ver (px) van de rand van de kaart, zodat hij altijd helemaal zichtbaar is.
constexpr double kMapMargin = 6.0;

struct SpawnEvent {
    const Enemy* enemy;
    EnemyType type;
};

struct FrameEvents {
    std::vector<SpawnEvent> spawned;
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
    // Handmatig een wave starten (Start Wave / Next Wave-knop). Geeft het bonusgoud terug, dat meteen
    // aan de speler wordt bijgeschreven. Er worden geen vijanden van eerdere waves verwijderd.
    bool canStartNextWave() const;
    int startNextWave();
    FrameEvents update(double dt);

    // --- Vlag van een Barracks ---
    // Een vlag mag alleen op de begaanbare weg (RouteBounds), binnen de kaart en niet op een bouwplek.
    bool canPlaceFlag(const Vec2& point) const;
    // Zet (of verplaatst) de vlag van de Barracks op deze bouwplek. False als er geen Barracks staat of als de
    // plek niet geldig is; de oude vlag blijft dan staan.
    bool placeFlag(std::size_t slot, const Vec2& point);
    // De vlag van de Barracks op deze bouwplek, of niets als daar geen Barracks staat.
    std::optional<Vec2> flagPosition(std::size_t slot) const;

    const Path& path() const;
    const RouteBounds& routeBounds() const;
    const Player& player() const;
    const WaveManager& waves() const;
    const std::vector<std::unique_ptr<Tower>>& towers() const;
    const std::vector<BuildSlot>& buildSlots() const;
    const std::vector<std::unique_ptr<Enemy>>& enemies() const;
    // Kogels die nu door de lucht vliegen. Hun schade valt pas bij de inslag (zie Projectile).
    const std::vector<Projectile>& projectiles() const;

    bool isGameOver() const;
    bool isWon() const;

private:
    void removeFinishedEnemies(FrameEvents& events);

    Path m_path;
    RouteBounds m_bounds;   // moet na m_path komen: wordt daaruit opgebouwd
    Player m_player;
    WaveManager m_waves;
    std::vector<BuildSlot> m_slots;
    std::vector<std::unique_ptr<Tower>> m_towers;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<Projectile> m_projectiles;
    std::uint64_t m_nextProjectileId = 1;
    std::unique_ptr<Spell> m_fireball = std::make_unique<Fireball>();
};

}

#endif  // KTD_GAME_H
