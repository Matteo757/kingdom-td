#ifndef KTD_ARTILLERYTOWER_H
#define KTD_ARTILLERYTOWER_H

#include <cmath>
#include "ktd/Tower.h"

namespace ktd {

class ArtilleryTower : public Tower {
public:
    static constexpr double kSplashRadius = 60.0;

    explicit ArtilleryTower(const Vec2& position)
        : Tower(position, 260.0, 45.0, DamageType::Physical, 3.0)
    {
    }

    void attack(Enemy& target) override
    {
        target.takeDamage({damage(), damageType()});
    }

    TowerType type() const override { return TowerType::Artillery; }

protected:
    void fire(Enemy& target, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) override
    {
        const Vec2 center = path.pointAt(target.distance());
        for (const auto& enemy : enemies) {
            if (enemy->isDead()) {
                continue;
            }
            const Vec2 d = path.pointAt(enemy->distance()) - center;
            if (std::hypot(d.x, d.y) <= kSplashRadius) {
                attack(*enemy);
            }
        }
    }
};

}

#endif  // KTD_ARTILLERYTOWER_H
