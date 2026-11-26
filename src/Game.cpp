#include "ktd/Game.h"
#include "ktd/BarracksTower.h"
#include "ktd/WaveFactory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

namespace ktd {
namespace {

// Alle coordinaten staan in sceneruimte (1024x576): de kaartafbeelding wordt op diezelfde grootte getekend.

// Level 1: vijanden komen vanaf de linkerrand en lopen naar het blauwe kasteel rechts (jouw basis).
// Level 1: vijanden komen vanaf de linkerrand en lopen over het kronkelpad naar de rechterrand.
Game makeLevelOne()
{
    std::vector<Vec2> route = {
        {0, 239},   {101, 241}, {225, 252}, {266, 243}, {359, 210}, {401, 205}, {438, 207},
        {476, 217}, {500, 236}, {513, 264}, {518, 309}, {526, 332}, {542, 351}, {570, 363},
        {628, 370}, {695, 357}, {730, 337}, {778, 283}, {799, 274}, {824, 271}, {855, 276},
        {882, 290}, {900, 311}, {925, 353}, {944, 369}, {966, 377}, {1024, 380}
    };
    // Eén bouwplek per bordje op de kaart (midden van de zandcirkel).
    std::vector<Vec2> slots = {
        {275, 165}, {356, 295}, {497, 415}, {514, 159}, {716, 205}, {827, 358}, {895, 446}
    };
    return Game(Path(std::move(route)), Player(1000, 20), WaveManager(buildWaves(10), 3.0, kWaveDelay, StartMode::Manual), std::move(slots));
}

// Level 3: van het kasteel linksboven over de brug naar de uitgang rechtsonder.
Game makeLevelThree()
{
    std::vector<Vec2> route = {
        {61, 107},  {77, 124},  {124, 146}, {205, 217}, {250, 224}, {323, 213}, {384, 213},
        {424, 224}, {457, 254}, {469, 293}, {464, 319}, {427, 350}, {352, 368}, {321, 393},
        {315, 409}, {317, 434}, {352, 474}, {404, 488}, {461, 486}, {557, 455}, {590, 438},
        {660, 430}, {752, 460}, {800, 456}, {859, 511}, {877, 511}, {929, 532}, {998, 533}
    };
    std::vector<Vec2> slots = {
        {173, 283}, {267, 154}, {360, 287}, {414, 426}, {506, 397}, {543, 168},
        {645, 500}, {672, 300}, {731, 167}, {764, 393}, {877, 268}
    };
    return Game(Path(std::move(route)), Player(220, 20), WaveManager(buildWaves(10), 3.0, kWaveDelay, StartMode::Manual), std::move(slots));
}

}

Game::Game(Path path, Player player, WaveManager waves, std::vector<Vec2> slotPositions)
    : m_path(std::move(path))
    , m_bounds(m_path)
    , m_player(player)
    , m_waves(std::move(waves))
{
    for (const Vec2& position : slotPositions) {
        m_slots.push_back({position, nullptr});
    }
}

Game Game::defaultLevel()
{
    return level(1);
}

Game Game::level(int number)
{
    switch (number) {
    case 1:
        return makeLevelOne();
    case 3:
        return makeLevelThree();
    default:
        throw std::invalid_argument("Unknown level");
    }
}

Game Game::fromLevel(const LevelData& data)
{
    // Path gooit std::invalid_argument bij minder dan twee waypoints.
    return Game(Path(data.route),
                Player(data.startGold, static_cast<std::uint8_t>(data.startLives)),
                WaveManager(buildWaves(data.waveCount), 3.0, kWaveDelay, StartMode::Manual),
                data.slotPositions);
}

void Game::addTower(std::unique_ptr<Tower> tower)
{
    m_towers.push_back(std::move(tower));
}

bool Game::canBuild(std::size_t slot, TowerType type) const
{
    return slot < m_slots.size() && m_slots[slot].tower == nullptr && m_player.gold() >= buildCost(type);
}

bool Game::buildTower(std::size_t slot, TowerType type)
{
    if (!canBuild(slot, type) || !m_player.spend(buildCost(type))) {
        return false;
    }
    addTower(makeTower(type, m_slots[slot].position, m_path));
    m_towers.back()->addInvestment(buildCost(type));   // canBuild() controleerde en spend() betaalde precies dit bedrag
    m_slots[slot].tower = m_towers.back().get();
    return true;
}

bool Game::canUpgrade(std::size_t slot) const
{
    if (slot >= m_slots.size() || m_slots[slot].tower == nullptr) {
        return false;
    }
    const Tower& tower = *m_slots[slot].tower;
    return tower.level() < Tower::kMaxLevel && m_player.gold() >= upgradeCost(tower.level());
}

bool Game::upgradeTower(std::size_t slot)
{
    if (!canUpgrade(slot)) {
        return false;
    }
    Tower& tower = *m_slots[slot].tower;
    const int cost = upgradeCost(tower.level());
    if (!m_player.spend(cost)) {
        return false;
    }
    tower.addInvestment(cost);   // het bedrag dat echt betaald is, voor de verkoopprijs
    tower.upgrade();
    return true;
}

bool Game::canSell(std::size_t slot) const
{
    return !isGameOver() && slot < m_slots.size() && m_slots[slot].tower != nullptr;
}

int Game::sellValue(std::size_t slot) const
{
    if (slot >= m_slots.size() || m_slots[slot].tower == nullptr) {
        return 0;
    }
    return sellRefund(m_slots[slot].tower->invested());
}

bool Game::sellTower(std::size_t slot)
{
    if (!canSell(slot)) {
        return false;   // geen toren (ook na een eerdere verkoop) of het spel is voorbij: geen tweede uitbetaling
    }
    Tower* tower = m_slots[slot].tower;
    const int refund = sellValue(slot);

    tower->onSold(m_enemies);   // laat bijvoorbeeld de vijanden van een Barracks weer lopen
    m_slots[slot].tower = nullptr;
    // Kogels die al in de lucht zijn blijven vliegen: ze verwijzen naar hun doel en niet naar de toren, en de
    // schade van een schot dat al gelost is hoort er gewoon bij.
    std::erase_if(m_towers, [tower](const std::unique_ptr<Tower>& t) { return t.get() == tower; });

    m_player.addGold(refund);   // het enige goudbeheer blijft Player
    return true;
}

bool Game::castFireball(const Vec2& target)
{
    return m_fireball->tryCast(m_enemies, m_path, target);
}

bool Game::fireballReady() const
{
    return m_fireball->ready();
}

bool Game::canStartNextWave() const
{
    return !isGameOver() && m_waves.canStartNextWave();
}

int Game::startNextWave()
{
    if (!canStartNextWave()) {
        return 0;
    }
    const int bonus = m_waves.startNextWave();
    if (bonus > 0) {
        m_player.addGold(bonus);
    }
    return bonus;
}

void Game::setPaused(bool paused)
{
    m_paused = paused;
}

bool Game::isPaused() const
{
    return m_paused;
}

FrameEvents Game::update(double dt)
{
    FrameEvents events;
    if (isGameOver() || m_paused) {
        return events;
    }

    m_fireball->update(dt);

    for (EnemyType type : m_waves.update(dt)) {
        m_enemies.push_back(makeEnemy(type));
        events.spawned.push_back({m_enemies.back().get(), type});
    }

    // Vereiste 5: useful and correct polymorphism (explain why)
    // Alle vijanden zitten als Enemy-pointer in één vector en alle torens als Tower-pointer;
    // update() en attack() roepen per object de juiste versie van Skeleton, Goblin, Orc, DarkMage en alle
    // torentypes aan, zonder dat deze lus de soorten hoeft te kennen.
    for (const auto& enemy : m_enemies) {
        enemy->update(dt);
    }

    // Een toren schiet een kogel af; de schade volgt pas als die kogel zijn doel bereikt.
    for (const auto& tower : m_towers) {
        std::optional<Projectile> shot = tower->update(dt, m_enemies, m_path);
        if (shot) {
            shot->setId(m_nextProjectileId++);
            m_projectiles.push_back(std::move(*shot));
        }
    }

    for (auto it = m_projectiles.begin(); it != m_projectiles.end();) {
        if (it->update(dt, m_enemies, m_path)) {
            it = m_projectiles.erase(it);   // geland: de schade is toegebracht
        } else {
            ++it;
        }
    }

    removeFinishedEnemies(events);
    return events;
}

void Game::removeFinishedEnemies(FrameEvents& events)
{
    for (auto it = m_enemies.begin(); it != m_enemies.end();) {
        const Enemy* enemy = it->get();
        const bool dead = enemy->isDead();
        const bool finished = enemy->distance() >= m_path.length();
        if (!dead && !finished) {
            ++it;
            continue;
        }
        if (dead) {
            m_player.addGold(enemy->reward());
            events.goldEarned += enemy->reward();
        } else {
            m_player.loseLife();
            ++events.livesLost;
        }
        for (Projectile& projectile : m_projectiles) {
            projectile.dropTarget(enemy);   // geen pointer naar een verwijderde vijand laten hangen
        }
        events.removed.push_back(enemy);
        it = m_enemies.erase(it);
    }
}

// --- Vlag van een Barracks ---

bool Game::canPlaceFlag(const Vec2& point) const
{
    return m_bounds.contains(point);
}

bool Game::canPlaceFlag(std::size_t slot, const Vec2& point) const
{
    if (slot >= m_slots.size() ||
        m_slots[slot].tower == nullptr ||
        !canPlaceFlag(point)) {
        return false;
    }

    const Tower& tower = *m_slots[slot].tower;

    return std::hypot(
        point.x - tower.position().x,
        point.y - tower.position().y
    ) <= tower.range();
}

bool Game::placeFlag(std::size_t slot, const Vec2& point)
{
    if (!canPlaceFlag(slot, point)) {
        return false;
    }

    auto* barracks = dynamic_cast<BarracksTower*>(m_slots[slot].tower);

    return barracks != nullptr && barracks->setFlag(point);
}

std::optional<Vec2> Game::flagPosition(std::size_t slot) const
{
    if (slot >= m_slots.size()) {
        return std::nullopt;
    }

    const auto* barracks = dynamic_cast<const BarracksTower*>(m_slots[slot].tower);

    if (barracks == nullptr) {
        return std::nullopt;
    }

    return barracks->flag();
}

const Path& Game::path() const { return m_path; }
const RouteBounds& Game::routeBounds() const { return m_bounds; }
const Player& Game::player() const { return m_player; }
const WaveManager& Game::waves() const { return m_waves; }
const std::vector<std::unique_ptr<Tower>>& Game::towers() const { return m_towers; }
const std::vector<BuildSlot>& Game::buildSlots() const { return m_slots; }
const std::vector<std::unique_ptr<Enemy>>& Game::enemies() const { return m_enemies; }
const std::vector<Projectile>& Game::projectiles() const { return m_projectiles; }

bool Game::isGameOver() const
{
    return m_player.isDead();
}

bool Game::isWon() const
{
    return !m_player.isDead() && m_waves.finished() && m_enemies.empty();
}

}