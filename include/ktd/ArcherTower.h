#ifndef KTD_ARCHERTOWER_H
#define KTD_ARCHERTOWER_H

#include "ktd/Tower.h"

namespace ktd {

class ArcherTower : public Tower {
public:
    explicit ArcherTower(const Vec2& position)
        : Tower(position, 150.0, 20.0, DamageType::Physical, 1.0)
    {
    }

    std::optional<Projectile> attack(Enemy& target, const Path& path) override
    {
        return makeProjectile(target, path);
    }

    TowerType type() const override { return TowerType::Archer; }
};

}

#endif  // KTD_ARCHERTOWER_H
