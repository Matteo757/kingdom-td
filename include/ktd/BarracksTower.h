#ifndef KTD_BARRACKSTOWER_H
#define KTD_BARRACKSTOWER_H

#include <cstddef>
#include <memory>
#include <vector>
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

    void attack(Enemy& target) override;
    void upgrade() override;
    Enemy* update(double dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path) override;
    TowerType type() const override { return TowerType::Barracks; }
    std::vector<Vec2> unitPositions() const override;

    const std::vector<Soldier>& soldiers() const;

private:
    bool hasFreeSoldier() const;

    double m_rally;
    Vec2 m_rallyPoint;
    std::vector<Soldier> m_soldiers;
};

}

#endif  // KTD_BARRACKSTOWER_H
