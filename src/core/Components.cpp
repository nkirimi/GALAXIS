// Components.cpp - Galaxis building blocks drawn with the CP1 algorithms.
#include "galaxis/Components.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include <cctype>

#include "galaxis/Font.h"
#include "galaxis/Texture.h"

namespace galaxis {

namespace {
int iround(double v) { return static_cast<int>(std::lround(v)); }
}  // namespace

// ====================================================================
// Orbit geometry
// ====================================================================
double orbitRadiusPx(double au) { return 40.0 + 95.0 * std::sqrt(au); }

Point orbitPoint(const OrbitView& v, double rx, double angle) {
    return {v.center.x + rx * std::cos(angle), v.center.y + rx * v.tilt * std::sin(angle)};
}

bool isNearSide(double angle) { return std::sin(angle) > 0.0; }

Vec3 lightFromOrbit(double angle, double tilt) {
    // Direction from the planet to the Sun in the orbital plane is
    // (-cos a, -sin a). The in-plane depth axis projects to screen y with
    // weight sin(elevation) = tilt and to z with weight cos(elevation).
    const double cosElev = std::sqrt(std::max(0.0, 1.0 - tilt * tilt));
    return normalize({-std::cos(angle), -std::sin(angle) * tilt, -std::sin(angle) * cosElev});
}

double planetAngle(const PlanetInfo& p, double day) { return p.phase0 - 2.0 * kPi * day / p.periodDays; }

// ====================================================================
// Background: vertical gradient, a faint nebula band, and a starfield.
// ====================================================================
void drawBackground(Canvas& c, unsigned seed) {
    const int W = c.width(), H = c.height();
    for (int y = 0; y < H; ++y) {
        const Color row = mix({3, 4, 12}, {8, 10, 24}, static_cast<double>(y) / H);
        for (int x = 0; x < W; ++x) {
            const double bandY = 0.30 * x + H * 0.12;
            const double dist = (y - bandY) / (H * 0.30);
            const double mask = std::exp(-dist * dist);
            const double n = fbm2(x / 170.0, y / 170.0, seed, 5);
            const double a = mask * smoothstep(0.38, 0.82, n) * 0.30;
            const Color neb = mix({70, 38, 115}, {28, 72, 135}, fbm2(x / 420.0 + 7, y / 420.0, seed + 9, 3));
            c.setPixel(x, y, mix(row, neb, a));
        }
    }

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    const int count = W * H / 850;
    for (int i = 0; i < count; ++i) {
        const int x = static_cast<int>(U(rng) * W);
        const int y = static_cast<int>(U(rng) * H);
        const double m = U(rng);
        const double t = U(rng);
        const double brightness = 0.15 + 0.85 * std::pow(m, 5.0);
        const Color tint = t < 0.15 ? Color{170, 195, 255} : (t < 0.25 ? Color{255, 215, 175} : colors::White);
        c.blendPixel(x, y, tint, brightness);

        if (m > 0.985) {  // bright star: four short fading spikes (Bresenham)
            const int len = 2 + static_cast<int>(U(rng) * 4);
            const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& d : dirs) {
                bresenhamLine(x, y, x + d[0] * len, y + d[1] * len, [&](int px, int py, int k) {
                    if (k > 0) c.addPixel(px, py, tint, 0.55 * (1.0 - static_cast<double>(k) / (len + 1)));
                });
            }
        }
    }
}

// ====================================================================
// Sun: additive glow (filled circle spans) + limb-darkened, granulated disc.
// ====================================================================
void drawSun(Canvas& c, Point center, int R, unsigned seed) {
    const int cx = iround(center.x), cy = iround(center.y);
    const int G = static_cast<int>(R * 3.2);

    circleSpans(cx, cy, G, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) {
            const double dx = x - cx, dy = y - cy;
            const double d = std::sqrt(dx * dx + dy * dy);
            const double t = std::max(0.0, d - R);
            double k = 0.85 * std::exp(-t / (0.35 * R)) + 0.22 * std::exp(-t / (1.3 * R));
            k *= clamp01((G - d) / (0.6 * R));  // fade out before the span edge
            c.addPixel(x, y, {255, 150, 60}, k);
        }
    });

    circleSpans(cx, cy, R, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) {
            const double dx = x - cx, dy = y - cy;
            const double d = std::sqrt(dx * dx + dy * dy);
            const double t = std::min(1.0, d / R);
            const double mu = std::sqrt(std::max(0.0, 1.0 - t * t));
            const Color base = mix({255, 250, 228}, {255, 160, 45}, std::pow(t, 1.6));
            const double granulation = 0.88 + 0.24 * fbm2(x / 3.5, y / 3.5, seed, 3);
            const double limb = 0.62 + 0.38 * mu;
            c.blendPixel(x, y, scale(base, granulation * limb * 1.05), clamp01(R + 0.5 - d));
        }
    });
}

// ====================================================================
// Planet: filled circle (midpoint spans) shaded per pixel as a sphere.
// ====================================================================
void drawPlanet(Canvas& c, const PlanetInfo& p, Point center, int radius, Vec3 lightDir, double rotation) {
    const Vec3 L = normalize(lightDir);
    const int cx = iround(center.x), cy = iround(center.y);
    const double rr = radius + 0.5;
    circleSpans(cx, cy, radius, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) {
            const double dx = x - cx, dy = y - cy;
            const double d2 = dx * dx + dy * dy;
            const double cover = clamp01(rr - std::sqrt(d2));  // anti-aliased edge
            if (cover <= 0.0) continue;

            // Surface normal of the sphere at this pixel.
            const double nz = std::sqrt(std::max(0.0, rr * rr - d2));
            const Vec3 N{dx / rr, dy / rr, nz / rr};

            const double lat = std::asin(std::clamp(-N.y, -1.0, 1.0));  // screen y is down
            const double lon = std::atan2(N.x, N.z) + rotation;
            const Color surf = surfaceColor(p, lon, lat);

            // Lambert diffuse with a soft terminator = visible day/night sides.
            const double lambert = dot(N, L);
            const double lit = 0.30 * smoothstep(-0.15, 0.25, lambert) + 0.70 * clamp01(lambert);
            Color col = scale(surf, (0.09 + 0.91 * lit) * (0.82 + 0.18 * N.z));

            if (p.atmosphereStrength > 0.0) {  // thin glowing limb on the day side
                const double rim = std::pow(1.0 - N.z, 2.2);
                col = add(col, scale(p.atmosphere, rim * p.atmosphereStrength * clamp01(lambert + 0.3)));
            }
            c.blendPixel(x, y, col, cover);
        }
    });
}

// ====================================================================
// Saturn's rings: an elliptical annulus filled scanline by scanline.
// Drawn in two halves so the planet can sit between them.
// ====================================================================
void drawRings(Canvas& c, Point center, int r, double tilt, Half half) {
    const int cx = iround(center.x), cy = iround(center.y);
    const double inner = kRingInner, outer = kRingOuter;  // in planet radii
    const double rxo = outer * r, ryo = rxo * tilt;

    const int yTop = (half == Half::Bottom) ? cy + 1 : cy - static_cast<int>(std::ceil(ryo));
    const int yBot = (half == Half::Top) ? cy : cy + static_cast<int>(std::ceil(ryo));
    for (int y = yTop; y <= yBot; ++y) {
        const double dy = y - cy;
        if (std::abs(dy) > ryo) continue;
        const double halfWidth = rxo * std::sqrt(1.0 - (dy / ryo) * (dy / ryo));  // outer ellipse
        for (int x = static_cast<int>(std::ceil(cx - halfWidth)); x <= static_cast<int>(std::floor(cx + halfWidth));
             ++x) {
            const double dx = x - cx;
            const double rho = std::sqrt(dx * dx + (dy / tilt) * (dy / tilt)) / r;  // radius in ring plane
            if (rho < inner || rho > outer) continue;
            c.blendPixel(x, y, ringColor(rho), ringAlpha(rho));
        }
    }
}

// ====================================================================
// Orbits, trails, selection marker
// ====================================================================
void drawOrbit(Canvas& c, const OrbitView& v, double rx, Color col, double alpha, Half half) {
    drawEllipse(c, iround(v.center.x), iround(v.center.y), iround(rx), iround(rx * v.tilt), col, alpha, half);
}

void drawTrail(Canvas& c, const OrbitView& v, double rx, double angle, double arc, Color col, double maxAlpha,
               Half half) {
    // The planet moves toward decreasing angle, so the trail lies at larger angles.
    const int segments = std::max(10, static_cast<int>(arc * rx / 3.0));
    for (int i = 0; i < segments; ++i) {
        const double a0 = angle + arc * i / segments;
        const double a1 = angle + arc * (i + 1) / segments;
        const bool near = isNearSide(0.5 * (a0 + a1));
        if ((half == Half::Top && near) || (half == Half::Bottom && !near)) continue;
        const Point p0 = orbitPoint(v, rx, a0), p1 = orbitPoint(v, rx, a1);
        const double fadeT = 1.0 - static_cast<double>(i) / segments;
        const double alpha = maxAlpha * std::pow(fadeT, 1.4);
        bresenhamLine(iround(p0.x), iround(p0.y), iround(p1.x), iround(p1.y), [&](int x, int y, int k) {
            if (k > 0 || i == 0) c.blendPixel(x, y, col, alpha);
        });
    }
}

// ====================================================================
// Panels
// ====================================================================
void drawPanel(Canvas& c, int x, int y, int w, int h, int ch, double fillAlpha) {
    const std::vector<Point> body = {{double(x + ch), double(y)}, {double(x + w), double(y)},
                                     {double(x + w), double(y + h - ch)}, {double(x + w - ch), double(y + h)},
                                     {double(x), double(y + h)}, {double(x), double(y + ch)}};
    scanlinePolygon(body, [&](int yy, int xl, int xr) {
        const Color bg = mix({16, 28, 56}, colors::HudBg, static_cast<double>(yy - y) / h);
        for (int xx = xl; xx <= xr; ++xx) c.blendPixel(xx, yy, bg, fillAlpha);
    });
    const std::vector<Point> edge = {{double(x + ch), double(y)}, {double(x + w - 1), double(y)},
                                     {double(x + w - 1), double(y + h - 1 - ch)},
                                     {double(x + w - 1 - ch), double(y + h - 1)}, {double(x), double(y + h - 1)},
                                     {double(x), double(y + ch)}};
    drawPolygon(c, edge, colors::HudLine, 0.8);
    drawLine(c, x, y + ch, x + ch, y, colors::Accent);  // accent on the cut corner
}

void drawAtmosphereHalo(Canvas& c, const PlanetInfo& p, Point center, int r, Vec3 lightDir) {
    if (p.atmosphereStrength <= 0.0 || r < 6) return;
    const Vec3 L = normalize(lightDir);
    const int cx = iround(center.x), cy = iround(center.y);
    const int outer = static_cast<int>(r * 1.12) + 2;
    circleSpans(cx, cy, outer, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) {
            const double dx = x - cx, dy = y - cy;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d <= r) continue;  // the disc itself is drawn by drawPlanet
            const double falloff = std::exp(-(d - r) / (0.035 * r));
            const double daySide = clamp01((dx * L.x + dy * L.y) / d + 0.35);  // limb normal . light
            c.addPixel(x, y, p.atmosphere, p.atmosphereStrength * 0.6 * falloff * daySide);
        }
    });
}

void drawCloseButton(Canvas& c, int cx, int cy, int r, double alpha) {
    fillCircle(c, cx, cy, r, {14, 22, 44}, 0.7 * alpha);
    drawCircle(c, cx, cy, r, colors::HudLine, 0.7 * alpha);
    const int k = static_cast<int>(r * 0.42);
    for (int o = 0; o <= 1; ++o) {  // the "x": two 2-pixel-wide DDA strokes
        drawLineDDA(c, cx - k + o, cy - k, cx + k + o, cy + k, colors::HudText, alpha);
        drawLineDDA(c, cx - k + o, cy + k, cx + k + o, cy - k, colors::HudText, alpha);
    }
}

void drawFactPanel(Canvas& c, int x, int y, int w, int h, const FactSheet& f, double alpha) {
    if (alpha <= 0.0) return;
    const double a = clamp01(alpha);
    const int ch = 18;

    // Panel body: chamfered polygon, scanline-filled with a vertical gradient.
    const std::vector<Point> body = {{double(x + ch), double(y)}, {double(x + w), double(y)},
                                     {double(x + w), double(y + h - ch)}, {double(x + w - ch), double(y + h)},
                                     {double(x), double(y + h)}, {double(x), double(y + ch)}};
    scanlinePolygon(body, [&](int yy, int xl, int xr) {
        const Color bg = mix({13, 20, 42}, {6, 9, 20}, static_cast<double>(yy - y) / h);
        for (int xx = xl; xx <= xr; ++xx) c.blendPixel(xx, yy, bg, 0.82 * a);
    });
    const std::vector<Point> edge = {{double(x + ch), double(y)}, {double(x + w - 1), double(y)},
                                     {double(x + w - 1), double(y + h - 1 - ch)},
                                     {double(x + w - 1 - ch), double(y + h - 1)}, {double(x), double(y + h - 1)},
                                     {double(x), double(y + ch)}};
    drawPolygon(c, edge, mix(f.accent, colors::HudLine, 0.5), 0.5 * a);
    fillPolygon(c, {{double(x), double(y + ch + 12)}, {double(x + 4), double(y + ch + 12)},
                    {double(x + 4), double(y + h - 40)}, {double(x), double(y + h - 40)}},
                f.accent, a);

    const int tx = x + 36;
    const int inner = w - 72;
    int ty = y + 34;

    std::string kind = f.kind;
    for (char& ch2 : kind) ch2 = static_cast<char>(std::toupper(static_cast<unsigned char>(ch2)));
    drawText(c, tx, ty, kind, f.accent, 2, a);
    ty += 26;
    drawText(c, tx, ty, f.name, colors::White, 6, a);
    ty += 42 + 22;
    drawLineDDA(c, tx, ty, tx + inner, ty, colors::HudLine, 0.35 * a);
    ty += 22;

    for (const auto& [label, value] : f.stats) {
        drawText(c, tx, ty, label, {140, 156, 192}, 2, a);
        ty += 22;
        const int vs = textWidth(value, 3) <= inner ? 3 : 2;  // shrink only if it would not fit
        drawText(c, tx, ty, value, {238, 242, 255}, vs, a);
        ty += 7 * vs + 18;
    }

    ty += 6;
    const std::vector<std::string> lines = wrapText(f.fact, inner - 18, 2);
    const int factHeight = static_cast<int>(lines.size()) * 22 - 6;
    fillPolygon(c, {{double(tx), double(ty)}, {double(tx + 3), double(ty)},
                    {double(tx + 3), double(ty + factHeight)}, {double(tx), double(ty + factHeight)}},
                f.accent, a);
    for (const std::string& line : lines) {
        drawText(c, tx + 18, ty, line, {215, 225, 245}, 2, a);
        ty += 22;
    }

    drawCloseButton(c, x + w - 38, y + 38, 14, a);
}

}  // namespace galaxis
