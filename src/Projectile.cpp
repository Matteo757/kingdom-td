#include "ktd/Projectile.h"

#include <algorithm>
#include <cmath>

namespace ktd {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

FlightProfile flightProfile(TowerType type)
{
    switch (type) {
    case TowerType::Archer:    return {325.0, 0.12, 10.0};
    case TowerType::Mage:      return {750.0, 0.0, 0.0};
    case TowerType::Cannon:    return {190.0, 0.0, 0.0};
    case TowerType::Artillery: return {190.0, 0.45, 60.0};
    case TowerType::Barracks:  return {300.0, 0.0, 0.0};   // vuurt niet, alleen voor de volledigheid
    }
    return {300.0, 0.0, 0.0};
}

Projectile::Projectile(TowerType source, std::uint8_t level, const Vec2& from, Enemy& target, const Path& path,
                       const Damage& damage, double splashRadius)
    : m_source(source)
    , m_level(std::clamp<std::uint8_t>(level, 1, 3))
    , m_from(from)
    , m_aim(path.pointAt(target.distance()))
    , m_position(from)
    , m_target(&target)
    , m_damage(damage)
    , m_splash(splashRadius)
{
    const FlightProfile profile = flightProfile(source);
    const Vec2 line = m_aim - m_from;
    const double distance = std::hypot(line.x, line.y);

    m_arcHeight = profile.arc > 0.0 ? std::max(profile.minArc, profile.arc * distance) : 0.0;
    // Een boog is langer dan de rechte lijn; de lengte van een vlakke parabool is bij benadering
    // d * (1 + 8/3 * (h/d)^2). Zo hangt de vliegtijd af van snelheid en boogtop.
    const double ratio = distance > 1.0 ? m_arcHeight / distance : 0.0;
    const double pathLength = distance * (1.0 + (8.0 / 3.0) * ratio * ratio);
    m_duration = std::max(0.05, pathLength / profile.speed);
}

bool Projectile::update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path)
{
    // Een doel dat al dood is krijgt geen tweede kogel meer; de kogel vliegt door naar de laatste plek.
    if (m_target != nullptr && m_target->isDead()) {
        m_target = nullptr;
    }
    if (m_target != nullptr) {
        m_aim = path.pointAt(m_target->distance());
    }

    m_elapsed += dt;
    const double t = std::min(m_elapsed / m_duration, 1.0);
    const Vec2 line = m_aim - m_from;

    // Positie = rechte lijn + een parabool omhoog (schermcoordinaten: omhoog = negatieve y), nul aan begin en eind.
    const double lift = 4.0 * m_arcHeight * t * (1.0 - t);
    m_position = {m_from.x + line.x * t, m_from.y + line.y * t - lift};

    // Richting = afgeleide van de baan: de kogel kantelt omhoog bij het wegschieten en omlaag bij de inslag.
    const double slope = 4.0 * m_arcHeight * (1.0 - 2.0 * t);
    const double vx = line.x;
    const double vy = line.y - slope;
    if (std::hypot(vx, vy) > 1e-6) {
        m_angle = std::atan2(vy, vx) * 180.0 / kPi;
    }

    if (t < 1.0) {
        return false;
    }
    land(enemies, path);
    return true;
}

void Projectile::land(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path)
{
    if (m_splash > 0.0) {
        for (const auto& enemy : enemies) {
            if (enemy->isDead()) {
                continue;
            }
            const Vec2 d = path.pointAt(enemy->distance()) - m_aim;
            if (std::hypot(d.x, d.y) <= m_splash) {
                enemy->takeDamage(m_damage);
            }
        }
    } else if (m_target != nullptr) {
        m_target->takeDamage(m_damage);
    }
}

void Projectile::dropTarget(const Enemy* enemy)
{
    if (m_target == enemy) {
        m_target = nullptr;
    }
}

void Projectile::setId(std::uint64_t id) { m_id = id; }
std::uint64_t Projectile::id() const { return m_id; }
TowerType Projectile::source() const { return m_source; }
std::uint8_t Projectile::level() const { return m_level; }
const Enemy* Projectile::target() const { return m_target; }
bool Projectile::hasTarget() const { return m_target != nullptr; }
double Projectile::splashRadius() const { return m_splash; }
const Vec2& Projectile::position() const { return m_position; }
double Projectile::angleDegrees() const { return m_angle; }
double Projectile::duration() const { return m_duration; }
double Projectile::progress() const { return std::min(m_elapsed / m_duration, 1.0); }

}
