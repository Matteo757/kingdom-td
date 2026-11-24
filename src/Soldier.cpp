#include "ktd/Soldier.h"

#include <algorithm>

namespace ktd {

Soldier::Soldier(double health, double damage, double interval, double respawnTime)
    : m_maxHealth(health)
    , m_health(health)
    , m_damage(damage)
    , m_respawnTime(respawnTime)
    , m_cooldown(interval)
{
}

void Soldier::update(double dt)
{
    if (!alive()) {
        m_respawn -= dt;
        if (m_respawn <= 0.0) {
            m_health = m_maxHealth;
        }
        return;
    }
    m_cooldown.update(dt);
    if (m_target == nullptr) {
        return;
    }
    if (m_cooldown.ready()) {
        m_target->takeDamage({m_damage, DamageType::Physical});
        m_cooldown.reset();
    }
    if (m_target->isDead()) {
        m_target = nullptr;
        return;
    }
    takeDamage(m_target->attackPower() * dt);
}

void Soldier::takeDamage(double amount)
{
    if (!alive()) {
        return;
    }
    m_health = std::max(0.0, m_health - amount);
    if (!alive()) {
        m_target = nullptr;
        m_respawn = m_respawnTime;
    }
}

void Soldier::engage(Enemy* enemy)
{
    m_target = enemy;
}

void Soldier::release()
{
    m_target = nullptr;
}

void Soldier::upgrade()
{
    m_maxHealth *= 1.25;
    m_damage *= 1.25;
    if (alive()) {
        m_health = m_maxHealth;
    }
}

void Soldier::place(double distance)
{
    m_distance = std::max(0.0, distance);
}

void Soldier::walkTo(double targetDistance, double dt)
{
    if (!alive() || dt <= 0.0) {
        return;
    }
    const double maxStep = kSpeed * dt;
    m_distance += std::clamp(targetDistance - m_distance, -maxStep, maxStep);
}

double Soldier::distance() const
{
    return m_distance;
}

bool Soldier::alive() const
{
    return m_health > 0.0;
}

double Soldier::health() const
{
    return m_health;
}

Enemy* Soldier::target() const
{
    return m_target;
}

}