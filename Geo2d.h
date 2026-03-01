#pragma once

#include <cmath>

namespace Geo2d
{
    struct Vector2
    {
        float x = 0.0f;
        float y = 0.0f;

        Vector2() = default;
        Vector2(float x_, float y_) : x(x_), y(y_) {}

        Vector2 operator+(const Vector2& o) const { return { x + o.x, y + o.y }; }
        Vector2 operator-(const Vector2& o) const { return { x - o.x, y - o.y }; }
        Vector2 operator*(float s) const { return { x * s, y * s }; }
        Vector2& operator+=(const Vector2& o) { x += o.x; y += o.y; return *this; }
        Vector2& operator-=(const Vector2& o) { x -= o.x; y -= o.y; return *this; }
        bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
        bool operator!=(const Vector2& o) const { return !(*this == o); }

        float length() const { return std::sqrt(x * x + y * y); }
        float distanceTo(const Vector2& o) const { return (*this - o).length(); }

        Vector2 normalized() const
        {
            float len = length();
            if (len > 0.0f) return { x / len, y / len };
            return { 0.0f, 0.0f };
        }

        static float distance(const Vector2& a, const Vector2& b)
        {
            return a.distanceTo(b);
        }
    };
}
