#ifndef KTD_MAGETOWER_H
#define KTD_MAGETOWER_H

#include "ktd/Tower.h"

namespace ktd {

class MageTower : public Tower {
public:
    explicit MageTower(const Vec2& position)
        : Tower(position, 140.0, 35.0, DamageType::Magic, 1.6)
    {
    }

    void attack(Enemy& target) override
    {
        target.takeDamage({damage(), damageType()});
    }

    TowerType type() const override { return TowerType::Mage; }
};

}

#endif  // KTD_MAGETOWER_H
