#ifndef KTD_GOBLIN_H
#define KTD_GOBLIN_H

#include "ktd/Enemy.h"

namespace ktd {

// Snel en zwak: komt in grote aantallen en is vooral gevaarlijk als de torens te traag zijn.
class Goblin : public Enemy {
public:
    Goblin() : Enemy(60.0, 0, 0) {}

    double speed() const override { return 110.0; }
    std::string name() const override { return "Goblin"; }
    int reward() const override { return 15; }
};

}

#endif  // KTD_GOBLIN_H
