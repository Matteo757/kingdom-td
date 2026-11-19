#pragma once

#include "ktd/Enemy.h"

namespace ktd {

// Heeft veel magic resist: een MageTower doet weinig, een ArcherTower werkt beter.
class DarkMage : public Enemy {
public:
    DarkMage() : Enemy(120.0, 0, 50) {}

    double speed() const override { return 55.0; }
    std::string name() const override { return "DarkMage"; }
    int reward() const override { return 30; }
};

}