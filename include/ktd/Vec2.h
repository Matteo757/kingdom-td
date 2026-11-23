#ifndef KTD_VEC2_H
#define KTD_VEC2_H

namespace ktd {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

inline Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(const Vec2& a, double f) { return {a.x * f, a.y * f}; }

}

#endif  // KTD_VEC2_H
