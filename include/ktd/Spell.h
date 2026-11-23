#ifndef KTD_SPELL_H
#define KTD_SPELL_H

#include <memory>
#include <vector>
#include "ktd/Cooldown.h"
#include "ktd/Enemy.h"
#include "ktd/Path.h"
#include "ktd/Vec2.h"

namespace ktd {

class Spell {
public:
    explicit Spell(double cooldown) : m_cooldown(cooldown) {}
    virtual ~Spell() = default;

    virtual void cast(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path, const Vec2& target) = 0;

    void update(double dt) { m_cooldown.update(dt); }
    bool ready() const { return m_cooldown.ready(); }

    bool tryCast(const std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path, const Vec2& target)
    {
        if (!ready()) {
            return false;
        }
        cast(enemies, path, target);
        m_cooldown.reset();
        return true;
    }

private:
    Cooldown<double> m_cooldown;
};

}

#endif  // KTD_SPELL_H
