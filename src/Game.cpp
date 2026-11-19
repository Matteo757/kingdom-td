#include "ktd/Game.h"

#include <utility>

namespace ktd {
namespace {

Wave makeWave(int skeletons, int goblins, int orcs, int darkMages)
{
    Wave wave;
    wave.spawnInterval = 1.2;
    while (skeletons > 0 || goblins > 0 || orcs > 0 || darkMages > 0) {
        if (skeletons-- > 0) wave.enemies.push_back(EnemyType::Skeleton);
        if (goblins-- > 0) wave.enemies.push_back(EnemyType::Goblin);
        if (orcs-- > 0) wave.enemies.push_back(EnemyType::Orc);
        if (darkMages-- > 0) wave.enemies.push_back(EnemyType::DarkMage);
    }
    return wave;
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
    std::vector<Wave> waves;
    for (int n = 1; n <= 10; ++n) {
        waves.push_back(makeWave(2 + n, n, n / 2, n / 3));
    }

    return Game(Path({{100, 100}, {400, 100}, {400, 300}, {800, 300}, {800, 480}}),
                Player(220, 20),
                WaveManager(std::move(waves), 3.0, 8.0),
                {{300, 170}, {600, 220}, {520, 360}, {480, 260}, {650, 430}});
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