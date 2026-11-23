#ifndef KTD_DAMAGE_H
#define KTD_DAMAGE_H

#include <cstdint>

namespace ktd {

enum class DamageType : std::uint8_t { Physical, Magic };

struct Damage {
    double amount;
    DamageType type;
};

}

#endif  // KTD_DAMAGE_H
