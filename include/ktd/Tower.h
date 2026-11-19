#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "ktd/Cooldown.h"
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/Path.h"
#include "ktd/Vec2.h"

namespace ktd {

class Tower {
public:
    static constexpr std::uint8_t kMaxLevel = 3;

    Tower(const Vec2& position, double range, double damage, DamageType type, double interval);
    virtual ~Tower() = default;

    virtual void attack(Enemy& target) = 0;
    virtual void upgrade();

    Enemy* findTarget(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) const;
    Enemy* update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path);

    const Vec2& position() const;
    double range() const;
    double damage() const;
    DamageType damageType() const;
    std::uint8_t level() const;

private:
    Vec2 m_position;
    double m_range;
    double m_damage;
    DamageType m_type;
    std::uint8_t m_level = 1;
    Cooldown<double> m_cooldown;
};

}