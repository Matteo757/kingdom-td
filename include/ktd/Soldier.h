#pragma once

#include "ktd/Cooldown.h"
#include "ktd/Enemy.h"

namespace ktd {

class Soldier {
public:
    Soldier(double health, double damage, double interval, double respawnTime);

    void update(double dt);
    void takeDamage(double amount);
    void engage(Enemy* enemy);
    void release();

    bool alive() const;
    double health() const;
    Enemy* target() const;

private:
    double m_maxHealth;
    double m_health;
    double m_damage;
    double m_respawnTime;
    double m_respawn = 0.0;
    Enemy* m_target = nullptr;
    Cooldown<double> m_cooldown;
};

}