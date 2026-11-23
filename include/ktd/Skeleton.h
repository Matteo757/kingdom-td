#ifndef KTD_SKELETON_H
#define KTD_SKELETON_H

#include "ktd/Enemy.h"

namespace ktd {

// Vereiste 4: useful and correct inheritance (explain why)
// Skeleton, Goblin, Orc en DarkMage erven alle gedeelde logica (schade, beweging, positie op het pad) van Enemy en voegen alleen toe wat ze uniek maakt: hun stats, snelheid en beloning.
class Skeleton : public Enemy {
public:
    Skeleton() : Enemy(100.0, 10, 0) {}

    double speed() const override { return 60.0; }
    std::string name() const override { return "Skeleton"; }
    int reward() const override { return 10; }
};

}

#endif  // KTD_SKELETON_H
