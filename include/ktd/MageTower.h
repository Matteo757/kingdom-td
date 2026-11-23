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

    std::optional<Projectile> attack(Enemy& target, const Path& path) override
    {
        return makeProjectile(target, path);
    }

    TowerType type() const override { return TowerType::Mage; }
};

}

#endif  // KTD_MAGETOWER_H
