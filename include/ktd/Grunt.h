#pragma once

#include "ktd/Enemy.h"

namespace ktd {

// Vereiste 4: useful and correct inheritance (explain why)
// Grunt erft alle gedeelde logica (schade, beweging) van Enemy en voegt alleen toe wat Grunt uniek maakt: zijn stats en snelheid.
class Grunt : public Enemy {
public:
    Grunt() : Enemy(100.0, 10, 0) {}

    double speed() const override { return 60.0; }
    std::string name() const override { return "Grunt"; }
};

}