#pragma once

#include <cstdint>

namespace ktd {

enum class DamageType : std::uint8_t { Physical, Magic };

struct Damage {
    double amount;
    DamageType type;
};

}