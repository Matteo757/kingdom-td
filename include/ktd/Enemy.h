#pragma once

#include <cstdint>
#include <string>
#include "ktd/Damage.h"

namespace ktd {

// Vereiste 8: useful and correct abstract base class
// Enemy heeft pure virtual functies (speed, name) en kan dus nooit zelf aangemaakt worden; het bundelt wat elke vijand gemeen heeft (health, armor, positie op het pad).
// Vereiste 3: useful and correct encapsulation (explain why)
// health, armor en magic resist zijn private; de enige manier om schade toe te brengen is takeDamage(), zodat de schadeformule nergens omzeild kan worden.
class Enemy {
public:
    Enemy(double health, std::uint8_t armor, std::uint8_t magicResist);
    virtual ~Enemy() = default;

    // Vereiste 9: useful and correct virtual function
    // speed() en name() zijn per vijandtype anders; via een Enemy-pointer wordt automatisch de juiste versie van Grunt of Runner opgeroepen.
    virtual double speed() const = 0;
    virtual std::string name() const = 0;

    void update(double dt);
    void takeDamage(const Damage& damage);

    double health() const;
    double distance() const;
    bool isDead() const;

private:
    double m_health;
    std::uint8_t m_armor;
    std::uint8_t m_magicResist;
    double m_distance = 0.0;
};

}