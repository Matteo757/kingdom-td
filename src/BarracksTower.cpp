#include "ktd/BarracksTower.h"

#include <algorithm>
#include <cmath>

namespace ktd {

namespace {

constexpr double kFormationSpacing = 16.0;   // afstand langs de weg tussen twee soldaten bij de vlag
constexpr double kSideOffset = 8.0;          // om en om links/rechts van het midden van de weg (past ruim in de weg)
constexpr double kFightGap = 12.0;           // een vechtende soldaat staat zo ver voor zijn vijand

}

BarracksTower::BarracksTower(const Vec2& position, const Path& path)
    : Tower(position, 100.0, 10.0, DamageType::Physical, 1.0)
    , m_path(path)
    , m_bounds(m_path)
    , m_homeDistance(path.closestDistance(position))
    , m_flagDistance(m_homeDistance)
    , m_flagPoint(path.pointAt(m_homeDistance))
{
    for (std::size_t i = 0; i < kSoldierCount; ++i) {
        m_soldiers.emplace_back(40.0, damage(), 1.0, 8.0);
        m_soldiers.back().place(m_flagDistance + formationOffset(i));
    }
}

double BarracksTower::formationOffset(std::size_t index) const
{
    return (static_cast<double>(index) - (static_cast<double>(kSoldierCount) - 1.0) / 2.0) * kFormationSpacing;
}

bool BarracksTower::setFlag(const Vec2& point)
{
    if (!m_bounds.contains(point)) {
        return false;   // niet op de weg: de oude vlag blijft staan
    }
    m_flagPoint = point;
    m_flagDistance = m_path.closestDistance(point);
    m_releaseFighters = true;
    return true;
}

const Vec2& BarracksTower::flag() const
{
    return m_flagPoint;
}

double BarracksTower::flagDistance() const
{
    return m_flagDistance;
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

// De barracks wordt verkocht: de vijanden die zijn soldaten op dat moment vasthouden mogen weer lopen. Anders
// zouden ze voor altijd geblokkeerd blijven, want niemand zou ze nog loslaten. Een soldaat kan nog een pointer
// hebben naar een vijand die net verwijderd is, dus we raken alleen vijanden aan die nog in de lijst staan.
void BarracksTower::onSold(const std::vector<std::unique_ptr<Enemy>>& enemies)
{
    for (Soldier& soldier : m_soldiers) {
        Enemy* target = soldier.target();
        const bool present = target != nullptr && std::any_of(enemies.begin(), enemies.end(),
            [target](const std::unique_ptr<Enemy>& e) { return e.get() == target; });
        if (present) {
            target->setBlocked(false);
        }
        soldier.release();
    }
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
        return std::abs(enemy.distance() - m_flagDistance) <= kReach;
    };

    // De vlag is verplaatst: vechtende soldaten laten hun vijand los (die loopt weer door) en gaan naar de nieuwe
    // vlag. Dit gebeurt hier en niet in setFlag(), omdat pas hier zeker is dat de vijand nog bestaat.
    if (m_releaseFighters) {
        for (Soldier& soldier : m_soldiers) {
            if (soldier.target() != nullptr && present(soldier.target())) {
                soldier.target()->setBlocked(false);
            }
            soldier.release();
        }
        m_releaseFighters = false;
    }

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

    for (std::size_t i = 0; i < m_soldiers.size(); ++i) {
        Soldier& soldier = m_soldiers[i];
        Enemy* target = soldier.target();
        const bool wasAlive = soldier.alive();
        soldier.update(dt);
        if (wasAlive && !soldier.alive() && target != nullptr) {
            target->setBlocked(false);
        }
        if (!wasAlive && soldier.alive()) {
            soldier.place(m_homeDistance + formationOffset(i));   // opnieuw geboren bij de barracks, loopt dan naar de vlag
        }
    }

    // Bewegen: langs de route (een afstand op het Path), dus bochten worden gevolgd en het gras blijft onaangeroerd.
    // Een soldaat die vecht loopt naar zijn vijand, alle anderen naar hun plek bij de vlag.
    for (std::size_t i = 0; i < m_soldiers.size(); ++i) {
        Soldier& soldier = m_soldiers[i];
        const Enemy* engaged = soldier.target();
        const double goal = engaged != nullptr ? engaged->distance() + kFightGap
                                               : m_flagDistance + formationOffset(i);
        soldier.walkTo(goal, dt);
    }
    return std::nullopt;
}

// Het punt op de weg waar de soldaat staat, met een kleine zijwaartse verschuiving zodat ze niet op elkaar staan.
// De uitkomst wordt altijd geklemd binnen de onzichtbare muren van de weg.
Vec2 BarracksTower::positionOf(const Soldier& soldier, std::size_t index) const
{
    const double distance = soldier.distance();
    const Vec2 center = m_path.pointAt(distance);
    const Vec2 ahead = m_path.pointAt(distance + 1.0);
    const Vec2 behind = m_path.pointAt(distance - 1.0);
    Vec2 direction{ahead.x - behind.x, ahead.y - behind.y};
    const double length = std::hypot(direction.x, direction.y);
    if (length == 0.0) {
        direction = {1.0, 0.0};
    } else {
        direction = {direction.x / length, direction.y / length};
    }
    const Vec2 normal{-direction.y, direction.x};
    const double side = (index % 2 == 0 ? 1.0 : -1.0) * kSideOffset;
    return m_bounds.clamp(center + normal * side);
}

std::vector<Vec2> BarracksTower::unitPositions() const
{
    std::vector<Vec2> positions;
    for (std::size_t i = 0; i < m_soldiers.size(); ++i) {
        if (m_soldiers[i].alive()) {
            positions.push_back(positionOf(m_soldiers[i], i));
        }
    }
    return positions;
}

const std::vector<Soldier>& BarracksTower::soldiers() const
{
    return m_soldiers;
}

}