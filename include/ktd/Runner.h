#pragma once

#include "ktd/Enemy.h"

namespace ktd {

class Runner : public Enemy {
public:
    Runner() : Enemy(60.0, 0, 0) {}

    double speed() const override { return 110.0; }
    std::string name() const override { return "Runner"; }
    int reward() const override { return 15; }
};

}