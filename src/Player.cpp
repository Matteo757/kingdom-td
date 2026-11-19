#include "ktd/Player.h"

namespace ktd {

Player::Player(int gold, std::uint8_t lives)
    : m_gold(gold)
    , m_lives(lives)
{
}

int Player::gold() const
{
    return m_gold;
}

std::uint8_t Player::lives() const
{
    return m_lives;
}

bool Player::isDead() const
{
    return m_lives == 0;
}

void Player::addGold(int amount)
{
    m_gold += amount;
}

bool Player::spend(int amount)
{
    if (amount > m_gold) {
        return false;
    }
    m_gold -= amount;
    return true;
}

void Player::loseLife()
{
    if (m_lives > 0) {
        --m_lives;
    }
}

}