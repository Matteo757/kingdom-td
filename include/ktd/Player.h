#pragma once

#include <cstdint>

namespace ktd {

class Player {
public:
    Player(int gold, std::uint8_t lives);

    int gold() const;
    std::uint8_t lives() const;
    bool isDead() const;

    void addGold(int amount);
    bool spend(int amount);
    void loseLife();

private:
    int m_gold;
    std::uint8_t m_lives;
};

}