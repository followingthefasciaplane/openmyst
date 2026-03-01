#pragma once

#include <random>
#include <cstdint>

namespace Rand
{
    // Get random int in range [min, max] (inclusive)
    inline int getInt(int min, int max)
    {
        static thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    // Get random float in range [min, max]
    inline float getFloat(float min, float max)
    {
        static thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    // Get random bool with probability p (0.0 to 1.0)
    inline bool chance(float p)
    {
        return getFloat(0.0f, 1.0f) < p;
    }

    // Get random percentage (0-99)
    inline int roll100()
    {
        return getInt(0, 99);
    }
}
