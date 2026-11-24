#ifndef KTD_SOLDIER_H
#define KTD_SOLDIER_H

#include "ktd/Cooldown.h"
#include "ktd/Enemy.h"

namespace ktd {

class Soldier {
public:
    // Loopsnelheid langs de weg in px per seconde.
    static constexpr double kSpeed = 90.0;

    Soldier(double health, double damage, double interval, double respawnTime);

    void update(double dt);
    void takeDamage(double amount);
    void engage(Enemy* enemy);
    void release();
    void upgrade();

    // Positie op de route, als afstand langs het Path (net als bij Enemy). De soldaat loopt dus altijd langs de weg.
    void place(double distance);
    void walkTo(double targetDistance, double dt);   // stapt met kSpeed naar targetDistance, zonder te overschieten

    bool alive() const;
    double health() const;
    double distance() const;
    Enemy* target() const;

private:
    double m_maxHealth;
    double m_health;
    double m_damage;
    double m_respawnTime;
    double m_respawn = 0.0;
    double m_distance = 0.0;
    Enemy* m_target = nullptr;
    Cooldown<double> m_cooldown;
};

}

#endif  // KTD_SOLDIER_H
