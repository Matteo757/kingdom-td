#pragma once

#include <cmath>
#include "ktd/Spell.h"

namespace ktd {

class Fireball : public Spell {
public:
    static constexpr double kRadius = 70.0;
    static constexpr double kDamage = 80.0;

    Fireball() : Spell(25.0) {}

    void cast(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path, const Vec2& target) override
    {
        for (const auto& enemy : enemies) {
            if (enemy->isDead()) {
                continue;
            }
            const Vec2 d = path.pointAt(enemy->distance()) - target;
            if (std::hypot(d.x, d.y) <= kRadius) {
                enemy->takeDamage({kDamage, DamageType::Magic});
            }
        }
    }
};

}