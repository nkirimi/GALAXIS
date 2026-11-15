// Texture.cpp - procedural noise and surface colours.
#include "galaxis/Texture.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace galaxis {

namespace {

std::uint32_t hashInt(int x, int y, int z, unsigned seed) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(y) * 668265263u +
                      static_cast<std::uint32_t>(z) * 2147483647u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

double hash01(int x, int y, int z, unsigned seed) {
    return static_cast<double>(hashInt(x, y, z, seed) & 0xFFFFFFu) / static_cast<double>(0xFFFFFFu);
}

double fade(double t) { return t * t * (3.0 - 2.0 * t); }

}  // namespace

double valueNoise3(double x, double y, double z, unsigned seed) {
    const int xi = static_cast<int>(std::floor(x));
    const int yi = static_cast<int>(std::floor(y));
    const int zi = static_cast<int>(std::floor(z));
    const double u = fade(x - xi), v = fade(y - yi), w = fade(z - zi);
    auto lerp1 = [](double a, double b, double t) { return a + (b - a) * t; };
    double c[2][2][2];
    for (int dz = 0; dz < 2; ++dz)
        for (int dy = 0; dy < 2; ++dy)
            for (int dx = 0; dx < 2; ++dx) c[dz][dy][dx] = hash01(xi + dx, yi + dy, zi + dz, seed);
    const double x00 = lerp1(c[0][0][0], c[0][0][1], u), x10 = lerp1(c[0][1][0], c[0][1][1], u);
    const double x01 = lerp1(c[1][0][0], c[1][0][1], u), x11 = lerp1(c[1][1][0], c[1][1][1], u);
    return lerp1(lerp1(x00, x10, v), lerp1(x01, x11, v), w);
}

double fbm3(double x, double y, double z, unsigned seed, int octaves) {
    double sum = 0, amp = 0.5, norm = 0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * valueNoise3(x, y, z, seed + 101u * static_cast<unsigned>(i));
        norm += amp;
        x *= 2.03;
        y *= 2.03;
        z *= 2.03;
        amp *= 0.5;
    }
    return sum / norm;
}

double fbm2(double x, double y, unsigned seed, int octaves) { return fbm3(x, y, 0.37, seed, octaves); }

unsigned planetSeed(const PlanetInfo& p) {
    return static_cast<unsigned>(p.name[0]) * 131u + static_cast<unsigned>(p.name.size());
}

Color surfaceColor(const PlanetInfo& p, double lon, double lat) {
    const unsigned seed = planetSeed(p);
    const double cl = std::cos(lat);
    const double X = cl * std::sin(lon), Y = std::sin(lat), Z = cl * std::cos(lon);

    switch (p.surface) {
        case Surface::Rocky: {
            const double n = fbm3(X * 2.6, Y * 2.6, Z * 2.6, seed, 4);
            const double fine = fbm3(X * 8, Y * 8, Z * 8, seed + 5, 2);
            return scale(mix(p.accent, p.base, smoothstep(0.35, 0.65, n)), 0.88 + 0.24 * fine);
        }
        case Surface::Banded: {
            const double turb = fbm3(X * 2.5, Y * 2.5, Z * 2.5, seed, 3);
            const double t = 0.5 + 0.5 * std::sin(Y * p.bandFreq + turb * 2.4);
            Color col = mix(p.base, p.accent, t * t);
            if (p.greatSpot) {
                const double dl = std::remainder(lon + 0.30, 2 * kPi);
                const double su = dl / 0.30, sv = (lat + 0.36) / 0.11;
                const double s = su * su + sv * sv;
                if (s < 1.0) col = mix(col, {195, 100, 72}, 0.9 * (1 - s));
            }
            return col;
        }
        case Surface::Ocean: {
            Color ground;
            const double n = fbm3(X * 1.8 + 3, Y * 1.8, Z * 1.8, seed, 6);
            if (n > 0.53)
                ground = mix(p.accent, {165, 145, 100}, smoothstep(0.53, 0.70, n));
            else
                ground = mix({18, 45, 120}, p.base, smoothstep(0.30, 0.53, n));
            const double capEdge = 0.91 + 0.06 * (fbm3(X * 4, Y * 4, Z * 4, seed + 7, 3) - 0.5);
            ground = mix(ground, {232, 238, 246}, smoothstep(capEdge - 0.03, capEdge + 0.03, std::abs(Y)));
            const double cloud = fbm3(X * 3 + 9, Y * 3, Z * 3, seed + 3, 6);
            return mix(ground, {242, 246, 252}, 0.85 * smoothstep(0.56, 0.74, cloud));
        }
        case Surface::Ice: {
            const double t = 0.5 + 0.5 * std::sin(Y * p.bandFreq + fbm3(X * 2, Y * 2, Z * 2, seed, 2) * 1.5);
            return mix(p.base, p.accent, 0.45 * t);
        }
    }
    return p.base;
}

Color sunSurfaceColor(double lon, double lat) {
    const double cl = std::cos(lat);
    const double X = cl * std::sin(lon), Y = std::sin(lat), Z = cl * std::cos(lon);
    const double g = fbm3(X * 14, Y * 14, Z * 14, 7, 4);  // granulation
    const double big = fbm3(X * 3 + 4, Y * 3, Z * 3, 11, 3);
    const double spots = smoothstep(0.74, 0.80, big) * smoothstep(0.5, 0.0, std::abs(Y) - 0.15);  // a few, near the equator
    const Color c = mix({255, 214, 120}, {255, 150, 45}, clamp01(0.5 + 1.4 * (g - 0.5)));
    return scale(c, (0.90 + 0.20 * g) * (1.0 - 0.35 * spots));
}

Color ringColor(double rho) {
    const double band = 0.5 + 0.5 * std::sin(rho * 26.0);
    return mix({190, 165, 122}, {238, 220, 178}, band);
}

double ringAlpha(double rho) {
    if (rho < kRingInner || rho > kRingOuter) return 0.0;
    double a = 0.82 * smoothstep(kRingInner, kRingInner + 0.08, rho) * (1.0 - smoothstep(kRingOuter - 0.08, kRingOuter, rho));
    if (rho > 1.86 && rho < 1.94) a *= 0.15;  // Cassini division
    return a;
}

Color skyColor(Vec3 d) {
    // A faint nebula band tilted across the sky, plus a very dark base.
    const Vec3 bandNormal = normalize({0.25, 0.9, 0.35});
    const double off = dot(d, bandNormal);
    const double mask = std::exp(-off * off / 0.06);
    const double n = fbm3(d.x * 2.2 + 5, d.y * 2.2, d.z * 2.2, 2026, 5);
    const double a = mask * smoothstep(0.38, 0.80, n) * 0.32;
    const Color neb = mix({70, 38, 115}, {28, 72, 135}, fbm3(d.x * 0.9, d.y * 0.9 + 3, d.z * 0.9, 2035, 3));
    const Color base = mix({3, 4, 12}, {7, 9, 22}, 0.5 + 0.5 * d.y);
    return mix(base, neb, a);
}

const std::vector<Star>& starCatalog() {
    static const std::vector<Star> kStars = [] {
        std::vector<Star> stars;
        std::mt19937 rng(2026);
        std::uniform_real_distribution<double> U(0.0, 1.0);
        for (int i = 0; i < 12000; ++i) {
            // Uniform direction on the sphere.
            const double z = 2 * U(rng) - 1;
            const double a = 2 * kPi * U(rng);
            const double r = std::sqrt(1 - z * z);
            const double m = U(rng);
            const double t = U(rng);
            const Color tint = t < 0.15 ? Color{170, 195, 255} : (t < 0.25 ? Color{255, 215, 175} : colors::White);
            stars.push_back({{r * std::cos(a), z, r * std::sin(a)}, 0.15 + 0.85 * std::pow(m, 6.0), tint});
        }
        return stars;
    }();
    return kStars;
}

}  // namespace galaxis
