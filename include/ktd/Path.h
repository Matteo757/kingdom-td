#pragma once

#include <vector>
#include "ktd/Vec2.h"

namespace ktd {

class Path {
public:
    explicit Path(std::vector<Vec2> waypoints);

    double length() const;
    Vec2 pointAt(double distance) const;
    const std::vector<Vec2>& waypoints() const;
    double closestDistance(const Vec2& point) const;

private:
    std::vector<Vec2> m_waypoints;
    std::vector<double> m_cumulative;
};

}