#pragma once

#include <cmath>
#include <algorithm>

namespace Util
{
    constexpr float PI = 3.14159265358979323846f;

    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    inline float clamp(float v, float lo, float hi) { return (std::max)(lo, (std::min)(v, hi)); }
    inline int clampInt(int v, int lo, int hi) { return (std::max)(lo, (std::min)(v, hi)); }

    inline float radToDeg(float rad) { return rad * 180.0f / PI; }
    inline float degToRad(float deg) { return deg * PI / 180.0f; }

    inline float orientationToRadians(float orientation)
    {
        return orientation * PI / 180.0f;
    }

    // Distance between two 2D points
    inline float distance2d(float x1, float y1, float x2, float y2)
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
}
