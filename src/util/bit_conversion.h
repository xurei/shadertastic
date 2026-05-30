#ifndef SHADERTASTIC_BIT_CONVERSION_H
#define SHADERTASTIC_BIT_CONVERSION_H

#include <cstdint>
#include <algorithm>
#include <cmath>

union float_and_int {
    float f;
    uint32_t u;
};

inline float pack1i1f(uint32_t in) {
    float_and_int a{.u = in};
    return a.f;
}

inline uint32_t pack2f1i(float a, float b)
{
    a = std::clamp(a, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);

    uint32_t ia = (uint32_t)std::round(a * 65535.0f);
    uint32_t ib = (uint32_t)std::round(b * 65535.0f);

    return (ia << 16) | ib;
}

#endif