#include "ktd/Tower.h"
#include "ktd/LaunchPoints.h"

#include <algorithm>
#include <cmath>

namespace ktd {

Tower::Tower(const Vec2& position, double range, double damage, DamageType type, double interval)
    : m_position(position)
    , m_range(range)
    , m_damage(damage)
    , m_type(type)
    , m_cooldown(interval)
{
}

void Tower::upgrade()
{
    if (m_level >= kMaxLevel) {
        return;
    }
    ++m_level;
    m_damage *= 1.25;
}

Enemy* Tower::findTarget(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) const
{
    const auto inRange = [&](const Enemy& enemy) {
        const Vec2 d = path.pointAt(enemy.distance()) - m_position;
        return !enemy.isDead() && std::hypot(d.x, d.y) <= m_range;
    };

    std::vector<Enemy*> candidates;
    for (const auto& enemy : enemies) {
        if (inRange(*enemy)) {
            candidates.push_back(enemy.get());
        }
    }
    if (candidates.empty()) {
        return nullptr;
    }
    return *std::max_element(candidates.begin(), candidates.end(),
        [](const Enemy* a, const Enemy* b) { return a->distance() < b->distance(); });
}

std::optional<Projectile> Tower::update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path)
{
    m_cooldown.update(dt);
    if (!m_cooldown.ready()) {
        return std::nullopt;
    }
    Enemy* target = findTarget(enemies, path);
    if (target == nullptr) {
        return std::nullopt;
    }
    std::optional<Projectile> shot = attack(*target, path);
    m_cooldown.reset();
    return shot;
}

Projectile Tower::makeProjectile(Enemy& target, const Path& path, double splashRadius)
{
    // Standaard (geen lanceerpunten) begint de kogel in het midden van de toren.
    Vec2 origin = m_position;
    const std::span<const LaunchPoint> points = launchPoints(type(), m_level);
    if (!points.empty()) {
        const LaunchPoint& point = points[m_nextShooter % points.size()];
        ++m_nextShooter;
        origin = {m_position.x + point.x, m_position.y + point.y};
    }
    return Projectile(type(), m_level, origin, target, path, {m_damage, m_type}, splashRadius);
}

std::vector<Vec2> Tower::unitPositions() const
{
    return {};
}

const Vec2& Tower::position() const { return m_position; }
double Tower::range() const { return m_range; }
double Tower::damage() const { return m_damage; }
DamageType Tower::damageType() const { return m_type; }
std::uint8_t Tower::level() const { return m_level; }

}