// Vereiste 13: 13. correct protections (for header files)
// Include guards (#ifndef/#define/#endif) voorkomen dat deze header twee keer in dezelfde vertaaleenheid terechtkomt.
#ifndef KTD_TOWER_H
#define KTD_TOWER_H

#include <cstdint>
#include <memory>
#include <vector>
#include "ktd/Cooldown.h"
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/Path.h"
#include "ktd/Vec2.h"

namespace ktd {

enum class TowerType : std::uint8_t { Archer, Cannon, Mage, Barracks, Artillery };

// Vereiste 8: useful and correct abstract base class
// Tower bundelt wat elke toren gemeen heeft (positie, bereik, schade, cooldown, level) en heeft pure virtual functies (attack, type); een toren zonder eigen gedrag kan dus nooit aangemaakt worden.
class Tower {
public:
    static constexpr std::uint8_t kMaxLevel = 3;

    Tower(const Vec2& position, double range, double damage, DamageType type, double interval);
    virtual ~Tower() = default;

    // Vereiste 9: useful and correct virtual function
    // attack() is per toren anders (pijl, kanonskogel, spreuk, soldaat); update() en fire() zijn virtual zodat BarracksTower en ArtilleryTower het schietgedrag van de basis kunnen vervangen.
    virtual void attack(Enemy& target) = 0;
    virtual TowerType type() const = 0;
    virtual void upgrade();
    virtual Enemy* update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path);
    virtual std::vector<Vec2> unitPositions() const;

    Enemy* findTarget(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) const;

    const Vec2& position() const;
    double range() const;
    double damage() const;
    DamageType damageType() const;
    std::uint8_t level() const;

protected:
    virtual void fire(Enemy& target, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path);

private:
    Vec2 m_position;
    double m_range;
    double m_damage;
    DamageType m_type;
    std::uint8_t m_level = 1;
    Cooldown<double> m_cooldown;
};

}

#endif  // KTD_TOWER_H
