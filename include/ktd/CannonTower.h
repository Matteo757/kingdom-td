#pragma once

#include "ktd/Tower.h"

namespace ktd {

class CannonTower : public Tower {
public:
    explicit CannonTower(const Vec2& position)
        : Tower(position, 120.0, 60.0, DamageType::Physical, 2.2)
    {
    }

    void attack(Enemy& target) override
    {
        target.takeDamage({damage(), damageType()});
    }

    TowerType type() const override { return TowerType::Cannon; }
};

}