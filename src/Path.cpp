#include "ktd/Path.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ktd {

Path::Path(std::vector<Vec2> waypoints)
    : m_waypoints(std::move(waypoints))
{
    if (m_waypoints.size() < 2) {
        throw std::invalid_argument("Path needs at least 2 waypoints");
    }
    m_cumulative.reserve(m_waypoints.size());
    m_cumulative.push_back(0.0);
    for (std::size_t i = 1; i < m_waypoints.size(); ++i) {
        const Vec2 d = m_waypoints[i] - m_waypoints[i - 1];
        m_cumulative.push_back(m_cumulative.back() + std::hypot(d.x, d.y));
    }
}

double Path::length() const
{
    return m_cumulative.back();
}

const std::vector<Vec2>& Path::waypoints() const
{
    return m_waypoints;
}

Vec2 Path::pointAt(double distance) const
{
    const double d = std::clamp(distance, 0.0, length());
    const auto it = std::upper_bound(m_cumulative.begin(), m_cumulative.end(), d);
    if (it == m_cumulative.end()) {
        return m_waypoints.back();
    }
    const std::size_t next = static_cast<std::size_t>(it - m_cumulative.begin());
    const std::size_t prev = next - 1;
    const double segment = m_cumulative[next] - m_cumulative[prev];
    if (segment == 0.0) {
        return m_waypoints[prev];
    }
    const double t = (d - m_cumulative[prev]) / segment;
    return m_waypoints[prev] + (m_waypoints[next] - m_waypoints[prev]) * t;
}

}