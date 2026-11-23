#include "ktd/RouteBounds.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ktd {

namespace {
constexpr double kTolerance = 1e-6;   // opvang voor afrondingsfouten bij punten precies op de muur
}

RouteBounds::RouteBounds(const Path& path, double halfWidth)
    : m_path(path)
    , m_halfWidth(halfWidth)
{
    if (halfWidth <= 0.0) {
        throw std::invalid_argument("Road half-width must be positive");
    }
}

Vec2 RouteBounds::closestOnRoute(const Vec2& point) const
{
    // Hergebruikt de bestaande Path-functies: dichtstbijzijnde afstand langs de route, daarna het punt daar.
    return m_path.pointAt(m_path.closestDistance(point));
}

double RouteBounds::distanceToRoute(const Vec2& point) const
{
    const Vec2 d = point - closestOnRoute(point);
    return std::hypot(d.x, d.y);
}

bool RouteBounds::contains(const Vec2& point) const
{
    return distanceToRoute(point) <= m_halfWidth + kTolerance;
}

Vec2 RouteBounds::clamp(const Vec2& point) const
{
    const Vec2 center = closestOnRoute(point);
    const Vec2 d = point - center;
    const double length = std::hypot(d.x, d.y);
    if (length <= m_halfWidth) {
        return point;
    }
    return center + d * (m_halfWidth / length);
}

Vec2 RouteBounds::moveWithin(const Vec2& from, const Vec2& to) const
{
    const Vec2 start = clamp(from);
    const Vec2 delta = to - start;
    const double length = std::hypot(delta.x, delta.y);
    // Kleine stappen (een achtste van de wegbreedte): zo kan een eenheid ook geen hoek afsnijden of
    // door een dunne muur "teleporteren".
    const double stepSize = m_halfWidth / 8.0;
    const int steps = std::max(1, static_cast<int>(std::ceil(length / stepSize)));
    const Vec2 step = delta * (1.0 / steps);
    // Elke stap vertrekt vanaf de vorige (geklemde) positie, zodat de beweging aaneengesloten blijft en
    // langs de muur schuift in plaats van te springen.
    Vec2 position = start;
    for (int i = 0; i < steps; ++i) {
        position = clamp(position + step);
    }
    return position;
}

bool RouteBounds::segmentInside(const Vec2& from, const Vec2& to) const
{
    const Vec2 delta = to - from;
    const double length = std::hypot(delta.x, delta.y);
    const int steps = std::max(1, static_cast<int>(std::ceil(length)));   // elke pixel één controle
    for (int i = 0; i <= steps; ++i) {
        if (!contains(from + delta * (static_cast<double>(i) / steps))) {
            return false;
        }
    }
    return true;
}

double RouteBounds::halfWidth() const
{
    return m_halfWidth;
}

}