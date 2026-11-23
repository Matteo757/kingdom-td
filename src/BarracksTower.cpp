#include "ktd/BarracksTower.h"

#include <algorithm>
#include <cmath>

namespace ktd {

BarracksTower::BarracksTower(const Vec2& position, const Path& path)
    : Tower(position, 100.0, 10.0, DamageType::Physical, 1.0)
    , m_rally(path.closestDistance(position))
    , m_rallyPoint(path.pointAt(m_rally))
    , m_bounds(path)
{
    for (std::size_t i = 0; i < kSoldierCount; ++i) {
        m_soldiers.emplace_back(40.0, damage(), 1.0, 8.0);
    }
}

bool BarracksTower::hasFreeSoldier() const
{
    return std::any_of(m_soldiers.begin(), m_soldiers.end(),
        [](const Soldier& soldier) { return soldier.alive() && soldier.target() == nullptr; });
}

// Een soldaat neemt de vijand aan: geen kogel, de soldaat doet de schade zelf.
std::optional<Projectile> BarracksTower::attack(Enemy& target, const Path&)
{
    for (Soldier& soldier : m_soldiers) {
        if (soldier.alive() && soldier.target() == nullptr) {
            soldier.engage(&target);
            target.setBlocked(true);
            break;
        }
    }
    return std::nullopt;
}

void BarracksTower::upgrade()
{
    const std::uint8_t before = level();
    Tower::upgrade();
    if (level() == before) {
        return;
    }
    for (Soldier& soldier : m_soldiers) {
        soldier.upgrade();
    }
}

std::optional<Projectile> BarracksTower::update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path)
{
    const auto present = [&](const Enemy* enemy) {
        return std::any_of(enemies.begin(), enemies.end(),
            [enemy](const std::unique_ptr<Enemy>& e) { return e.get() == enemy; });
    };
    const auto inReach = [&](const Enemy& enemy) {
        return std::abs(enemy.distance() - m_rally) <= kReach;
    };

    for (Soldier& soldier : m_soldiers) {
        if (soldier.target() != nullptr && !present(soldier.target())) {
            soldier.release();
        }
    }

    for (const auto& enemy : enemies) {
        if (!hasFreeSoldier()) {
            break;
        }
        if (!enemy->isDead() && !enemy->isBlocked() && inReach(*enemy)) {
            attack(*enemy, path);
        }
    }

    for (Soldier& soldier : m_soldiers) {
        Enemy* target = soldier.target();
        const bool wasAlive = soldier.alive();
        soldier.update(dt);
        if (wasAlive && !soldier.alive() && target != nullptr) {
            target->setBlocked(false);
        }
    }
    return std::nullopt;
}

std::vector<Vec2> BarracksTower::unitPositions() const
{
    std::vector<Vec2> positions;
    for (std::size_t i = 0; i < m_soldiers.size(); ++i) {
        if (m_soldiers[i].alive()) {
            const Vec2 wanted{m_rallyPoint.x + (static_cast<double>(i) - 1.0) * 16.0, m_rallyPoint.y + 16.0};
            positions.push_back(m_bounds.clamp(wanted));   // nooit buiten de weg
        }
    }
    return positions;
}

const std::vector<Soldier>& BarracksTower::soldiers() const
{
    return m_soldiers;
}

}