#include "ktd/Game.h"
#include "ktd/WaveFactory.h"

#include <stdexcept>
#include <utility>

namespace ktd {
namespace {

// Alle coordinaten staan in sceneruimte (1024x576): de kaartafbeelding wordt op diezelfde grootte getekend.

// Level 1: vijanden komen vanaf de linkerrand en lopen naar het blauwe kasteel rechts (jouw basis).
Game makeLevelOne()
{
    std::vector<Vec2> route = {
        {0, 236},   {36, 245},  {135, 230}, {177, 231}, {262, 262}, {317, 267}, {426, 232},
        {473, 231}, {538, 250}, {641, 304}, {695, 315}, {739, 317}, {845, 292}, {969, 298},
        {985, 293}
    };
    // Eén bouwplek per bordje op de kaart (midden van de zandcirkel).
    std::vector<Vec2> slots = {
        {177, 307}, {291, 189}, {449, 302}, {573, 202}, {610, 370}, {771, 242}, {851, 371}
    };
    return Game(Path(std::move(route)), Player(220, 20), WaveManager(buildWaves(10), 3.0, kWaveDelay), std::move(slots));
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
    return Game(Path(std::move(route)), Player(220, 20), WaveManager(buildWaves(10), 3.0, kWaveDelay), std::move(slots));
}

}

Game::Game(Path path, Player player, WaveManager waves, std::vector<Vec2> slotPositions)
    : m_path(std::move(path))
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
    if (!m_player.spend(upgradeCost(tower.level()))) {
        return false;
    }
    tower.upgrade();
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

FrameEvents Game::update(double dt)
{
    FrameEvents events;
    if (isGameOver()) {
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

    for (const auto& tower : m_towers) {
        const Enemy* target = tower->update(dt, m_enemies, m_path);
        if (target != nullptr) {
            events.shots.push_back({tower->position(), m_path.pointAt(target->distance()), tower->damageType()});
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
        events.removed.push_back(enemy);
        it = m_enemies.erase(it);
    }
}

const Path& Game::path() const { return m_path; }
const Player& Game::player() const { return m_player; }
const WaveManager& Game::waves() const { return m_waves; }
const std::vector<std::unique_ptr<Tower>>& Game::towers() const { return m_towers; }
const std::vector<BuildSlot>& Game::buildSlots() const { return m_slots; }
const std::vector<std::unique_ptr<Enemy>>& Game::enemies() const { return m_enemies; }

bool Game::isGameOver() const
{
    return m_player.isDead();
}

bool Game::isWon() const
{
    return !m_player.isDead() && m_waves.finished() && m_enemies.empty();
}

}