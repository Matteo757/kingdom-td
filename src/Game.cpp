#include "ktd/Game.h"

#include <utility>
#include "ktd/ArcherTower.h"
#include "ktd/MageTower.h"

namespace ktd {

Game::Game(Path path, Player player, WaveManager waves)
    : m_path(std::move(path))
    , m_player(player)
    , m_waves(std::move(waves))
{
}

Game Game::defaultLevel()
{
    std::vector<Wave> waves;
    for (int n = 1; n <= 10; ++n) {
        Wave wave;
        wave.spawnInterval = 1.2;
        for (int i = 0; i < 3 + n; ++i) {
            wave.enemies.push_back(EnemyType::Grunt);
            if (i % 3 == 2) {
                wave.enemies.push_back(EnemyType::Runner);
            }
        }
        waves.push_back(std::move(wave));
    }

    Game game(Path({{100, 100}, {400, 100}, {400, 300}, {800, 300}, {800, 480}}),
              Player(100, 20),
              WaveManager(std::move(waves), 3.0, 8.0));
    game.addTower(std::make_unique<ArcherTower>(Vec2{300, 170}));
    game.addTower(std::make_unique<MageTower>(Vec2{600, 220}));
    return game;
}

void Game::addTower(std::unique_ptr<Tower> tower)
{
    m_towers.push_back(std::move(tower));
}

FrameEvents Game::update(double dt)
{
    FrameEvents events;
    if (isGameOver()) {
        return events;
    }

    for (EnemyType type : m_waves.update(dt)) {
        m_enemies.push_back(makeEnemy(type));
        events.spawned.push_back({m_enemies.back().get(), type});
    }

    // Vereiste 5: useful and correct polymorphism (explain why)
    // Alle vijanden zitten als Enemy-pointer in één vector en alle torens als Tower-pointer;
    // update() en attack() roepen per object de juiste versie van Grunt, Runner, ArcherTower of
    // MageTower aan, zonder dat deze lus de soorten hoeft te kennen.
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