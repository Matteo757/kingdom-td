#include "ktd/Enemy.h"

#include <algorithm>

namespace ktd {

Enemy::Enemy(double health, std::uint8_t armor, std::uint8_t magicResist)
    : m_health(health)
    , m_armor(std::min<std::uint8_t>(armor, 100))
    , m_magicResist(std::min<std::uint8_t>(magicResist, 100))
{
}

void Enemy::update(double dt)
{
    if (isDead()) {
        return;
    }
    m_distance += speed() * dt;
}

void Enemy::takeDamage(const Damage& damage)
{
    const double resist = damage.type == DamageType::Physical ? m_armor : m_magicResist;
    const double dealt = damage.amount * (100.0 - resist) / 100.0;
    m_health = std::max(0.0, m_health - dealt);
}

double Enemy::health() const
{
    return m_health;
}

double Enemy::distance() const
{
    return m_distance;
}

bool Enemy::isDead() const
{
    return m_health <= 0.0;
}

}