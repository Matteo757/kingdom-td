#ifndef KTD_ORC_H
#define KTD_ORC_H

#include "ktd/Enemy.h"

namespace ktd {

// Traag maar zwaar gepantserd: fysieke schade (pijlen) doet weinig, magie wel.
class Orc : public Enemy {
public:
    Orc() : Enemy(250.0, 40, 0) {}

    double speed() const override { return 40.0; }
    std::string name() const override { return "Orc"; }
    int reward() const override { return 25; }
};

}

#endif  // KTD_ORC_H
