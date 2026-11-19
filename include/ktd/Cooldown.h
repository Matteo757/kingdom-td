#pragma once

#include <algorithm>

namespace ktd {

template <typename T>
class Cooldown {
public:
    explicit Cooldown(T interval) : m_interval(interval), m_remaining(0) {}

    void update(T dt) { m_remaining = std::max<T>(0, m_remaining - dt); }
    bool ready() const { return m_remaining <= 0; }
    void reset() { m_remaining = m_interval; }

private:
    T m_interval;
    T m_remaining;
};

}