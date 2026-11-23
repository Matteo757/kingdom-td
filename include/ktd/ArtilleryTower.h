#ifndef KTD_ARTILLERYTOWER_H
#define KTD_ARTILLERYTOWER_H

#include "ktd/Tower.h"

namespace ktd {

class ArtilleryTower : public Tower {
public:
    static constexpr double kSplashRadius = 60.0;

    explicit ArtilleryTower(const Vec2& position)
        : Tower(position, 260.0, 45.0, DamageType::Physical, 3.0)
    {
    }

    // De raket ontploft bij de inslag en doet schade aan alle vijanden binnen kSplashRadius van het inslagpunt.
    std::optional<Projectile> attack(Enemy& target, const Path& path) override
    {
        return makeProjectile(target, path, kSplashRadius);
    }

    TowerType type() const override { return TowerType::Artillery; }
};

}

#endif  // KTD_ARTILLERYTOWER_H
