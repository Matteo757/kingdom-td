// Vereiste 13: 13. correct protections (for header files)
// Include guards (#ifndef/#define/#endif) voorkomen dat deze header twee keer in dezelfde vertaaleenheid terechtkomt.
#ifndef KTD_TOWER_H
#define KTD_TOWER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include "ktd/Cooldown.h"
#include "ktd/Damage.h"
#include "ktd/Enemy.h"
#include "ktd/Path.h"
#include "ktd/Projectile.h"
#include "ktd/TowerType.h"
#include "ktd/Vec2.h"

namespace ktd {

// Vereiste 8: useful and correct abstract base class
// Tower bundelt wat elke toren gemeen heeft (positie, bereik, schade, cooldown, level) en heeft pure virtual functies (attack, type); een toren zonder eigen gedrag kan dus nooit aangemaakt worden.
class Tower {
public:
    static constexpr std::uint8_t kMaxLevel = 3;

    Tower(const Vec2& position, double range, double damage, DamageType type, double interval);
    virtual ~Tower() = default;

    // Vereiste 9: useful and correct virtual function
    // attack() is per toren anders (pijl, kanonskogel, magiebol, raket met splash, soldaat); update() is virtual zodat BarracksTower het schietgedrag van de basis kan vervangen.
    // Een aanval levert een kogel op die pas bij de inslag schade doet; de barracks levert geen kogel (de soldaten vechten zelf).
    virtual std::optional<Projectile> attack(Enemy& target, const Path& path) = 0;
    virtual TowerType type() const = 0;
    virtual void upgrade();
    virtual std::optional<Projectile> update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path);
    virtual std::vector<Vec2> unitPositions() const;

    // Wordt aangeroepen vlak voordat de toren verkocht en verwijderd wordt, zodat hij kan opruimen wat buiten
    // hem om blijft bestaan. Standaard hoeft dat niet; BarracksTower laat er zijn vastgehouden vijanden mee los.
    virtual void onSold(const std::vector<std::unique_ptr<Enemy>>& enemies);

    Enemy* findTarget(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) const;

    const Vec2& position() const;
    double range() const;
    double damage() const;
    DamageType damageType() const;
    std::uint8_t level() const;

    // Totaal goud dat in deze ene toren gestoken is: de bouwprijs plus de werkelijk betaalde prijs van elke
    // upgrade. Game schrijft die bedragen bij; de verkoopprijs wordt hieruit berekend (niet uit het level).
    int invested() const;
    void addInvestment(int gold);

protected:
    // Maakt de kogel van deze toren met zijn huidige schade en level; splashRadius > 0 geeft schade in een gebied.
    // De kogel begint op een lanceerpunt van de toren (zie LaunchPoints.h); heeft de toren meerdere punten,
    // dan wisselen die elkaar bij elk schot af. Daarom is deze functie niet const.
    Projectile makeProjectile(Enemy& target, const Path& path, double splashRadius = 0.0);

private:
    Vec2 m_position;
    double m_range;
    double m_damage;
    DamageType m_type;
    std::uint8_t m_level = 1;
    int m_invested = 0;
    std::size_t m_nextShooter = 0;   // welk lanceerpunt het volgende schot krijgt
    Cooldown<double> m_cooldown;
};

}

#endif  // KTD_TOWER_H
