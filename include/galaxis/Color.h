// Color.h - 24-bit RGB color and small color utilities.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace galaxis {

struct Color {
    std::uint8_t r = 0, g = 0, b = 0;
    constexpr Color() = default;
    constexpr Color(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_) : r(r_), g(g_), b(b_) {}
};

inline bool operator==(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
inline bool operator!=(Color a, Color b) { return !(a == b); }

// Clamp a double to [0,255] and round to the nearest byte.
inline std::uint8_t clampByte(double v) {
    return static_cast<std::uint8_t>(std::clamp(v, 0.0, 255.0) + 0.5);
}

// Linear interpolation between two colors (t = 0 -> a, t = 1 -> b).
inline Color mix(Color a, Color b, double t) {
    return {clampByte(a.r + (b.r - a.r) * t),
            clampByte(a.g + (b.g - a.g) * t),
            clampByte(a.b + (b.b - a.b) * t)};
}

// Multiply every channel by k (brightness).
inline Color scale(Color c, double k) {
    return {clampByte(c.r * k), clampByte(c.g * k), clampByte(c.b * k)};
}

// Saturating add.
inline Color add(Color a, Color b) {
    return {clampByte(double(a.r) + b.r), clampByte(double(a.g) + b.g), clampByte(double(a.b) + b.b)};
}

// HSV -> RGB. h, s, v all in [0,1].
inline Color hsv(double h, double s, double v) {
    h = h - std::floor(h);
    const double hh = h * 6.0;
    const int i = static_cast<int>(hh);
    const double f = hh - i;
    const double p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    double r = 0, g = 0, b = 0;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return {clampByte(r * 255), clampByte(g * 255), clampByte(b * 255)};
}

// Shared palette for the Galaxis look.
namespace colors {
constexpr Color Black{0, 0, 0};
constexpr Color White{255, 255, 255};
constexpr Color Space{4, 6, 14};
constexpr Color HudBg{10, 16, 32};
constexpr Color HudLine{80, 150, 255};
constexpr Color HudText{205, 225, 255};
constexpr Color HudDim{110, 130, 170};
constexpr Color Accent{255, 190, 70};
constexpr Color Orbit{95, 120, 175};
}  // namespace colors

}  // namespace galaxis
