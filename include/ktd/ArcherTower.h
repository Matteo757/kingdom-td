#pragma once

#include "ktd/Tower.h"

namespace ktd {

class ArcherTower : public Tower {
public:
    explicit ArcherTower(const Vec2& position)
        : Tower(position, 150.0, 20.0, DamageType::Physical, 1.0)
    {
    }

    void attack(Enemy& target) override
    {
        target.takeDamage({damage(), damageType()});
    }

    TowerType type() const override { return TowerType::Archer; }
};

}