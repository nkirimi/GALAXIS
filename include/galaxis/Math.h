// Math.h - small vector types shared by every checkpoint.
#pragma once

#include <cmath>

namespace galaxis {

constexpr double kPi = 3.14159265358979323846;

inline double radians(double deg) { return deg * kPi / 180.0; }

// ------------------------------------------------------------------ 2-D
struct Point {
    double x = 0, y = 0;
};

inline Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
inline Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
inline Point operator*(Point a, double k) { return {a.x * k, a.y * k}; }

// ------------------------------------------------------------------ 3-D
struct Vec3 {
    double x = 0, y = 0, z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, double k) { return {a.x * k, a.y * k, a.z * k}; }
inline Vec3 operator*(double k, Vec3 a) { return a * k; }
inline Vec3& operator+=(Vec3& a, Vec3 b) { return a = a + b; }

inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double length(Vec3 v) { return std::sqrt(dot(v, v)); }

inline Vec3 normalize(Vec3 v) {
    const double len = length(v);
    if (len == 0) return v;
    return {v.x / len, v.y / len, v.z / len};
}

inline Vec3 lerp(Vec3 a, Vec3 b, double t) { return a + (b - a) * t; }

struct Vec4 {
    double x = 0, y = 0, z = 0, w = 0;
};

// --------------------------------------------------------------- scalar
inline double clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

inline double smoothstep(double e0, double e1, double x) {
    const double t = clamp01((x - e0) / (e1 - e0));
    return t * t * (3 - 2 * t);
}

inline double easeInOutCubic(double t) {
    t = clamp01(t);
    return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}

// ------------------------------------------------------------- Bezier
// Quadratic and cubic Bezier curves (de Casteljau form written out).
inline Point bezier(Point p0, Point p1, Point p2, double t) {
    const double u = 1 - t;
    return p0 * (u * u) + p1 * (2 * u * t) + p2 * (t * t);
}

inline Vec3 bezier(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, double t) {
    const double u = 1 - t;
    return p0 * (u * u * u) + p1 * (3 * u * u * t) + p2 * (3 * u * t * t) + p3 * (t * t * t);
}

}  // namespace galaxis
