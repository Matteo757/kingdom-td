#ifndef KTD_BARRACKSTOWER_H
#define KTD_BARRACKSTOWER_H

#include <cstddef>
#include <memory>
#include <vector>
#include "ktd/RouteBounds.h"
#include "ktd/Soldier.h"
#include "ktd/Tower.h"

namespace ktd {

// Vereiste 6: useful and correct object composition (explain why)
// BarracksTower bevat zijn Soldiers (has-a): ze bestaan alleen zolang de toren bestaat, worden door de toren aangestuurd en hebben buiten de toren geen eigen levensduur nodig.
class BarracksTower : public Tower {
public:
    static constexpr std::size_t kSoldierCount = 3;
    static constexpr double kReach = 40.0;

    BarracksTower(const Vec2& position, const Path& path);

    std::optional<Projectile> attack(Enemy& target, const Path& path) override;
    void upgrade() override;
    std::optional<Projectile> update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) override;
    TowerType type() const override { return TowerType::Barracks; }
    std::vector<Vec2> unitPositions() const override;
    void onSold(const std::vector<std::unique_ptr<Enemy>>& enemies) override;

    const std::vector<Soldier>& soldiers() const;

    // --- De vlag (rally point) ---
    // De soldaten lopen naar de vlag en bewaken het stuk weg eromheen. Een nieuwe barracks heeft zijn vlag op het
    // punt van de weg dat het dichtst bij de toren ligt.
    // setFlag() weigert (false) alles wat niet binnen de begaanbare weg (RouteBounds) ligt. Waar de vlag verder
    // niet mag staan (buiten de kaart, op een bouwplek) beslist Game::canPlaceFlag().
    bool setFlag(const Vec2& point);
    const Vec2& flag() const;
    double flagDistance() const;   // positie van de vlag langs de route

private:
    bool hasFreeSoldier() const;
    double formationOffset(std::size_t index) const;     // afstand langs de weg ten opzichte van de vlag
    Vec2 positionOf(const Soldier& soldier, std::size_t index) const;

    Path m_path;
    RouteBounds m_bounds;   // de onzichtbare muren: soldaten blijven binnen de weg
    double m_homeDistance;  // het punt van de weg bij de barracks zelf (hier verschijnt een nieuwe soldaat)
    double m_flagDistance;
    Vec2 m_flagPoint;
    bool m_releaseFighters = false;   // na een verplaatste vlag laten vechtende soldaten hun vijand los
    std::vector<Soldier> m_soldiers;
};

}

#endif  // KTD_BARRACKSTOWER_H
