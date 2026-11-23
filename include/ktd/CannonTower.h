#ifndef KTD_CANNONTOWER_H
#define KTD_CANNONTOWER_H

#include "ktd/Tower.h"

namespace ktd {

class CannonTower : public Tower {
public:
    explicit CannonTower(const Vec2& position)
        : Tower(position, 120.0, 60.0, DamageType::Physical, 2.2)
    {
    }

    std::optional<Projectile> attack(Enemy& target, const Path& path) override
    {
        return makeProjectile(target, path);
    }

    TowerType type() const override { return TowerType::Cannon; }
};

}

#endif  // KTD_CANNONTOWER_H
