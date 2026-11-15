// Renderer3D.cpp - CP2: the Galaxis world drawn in true 3-D with the CP1
// rasterisers and the CP2 transformation matrices.
#include "galaxis/Renderer3D.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "galaxis/Clip.h"
#include "galaxis/Components.h"
#include "galaxis/Raster.h"
#include "galaxis/Texture.h"

namespace galaxis {

namespace {

int iround(double v) { return static_cast<int>(std::lround(v)); }

}  // namespace

// ================================================================ projector
Projector::Projector(const CameraPose& p, double fovY, int w, int h) : width(w), height(h), pose(p) {
    view = p.view();
    proj = perspective(fovY, static_cast<double>(w) / h, kNear, kFar);
    viewProj = proj * view;
    screen = galaxis::viewport(w, h) * viewProj;
    focalPx = (h / 2.0) / std::tan(fovY / 2);
}

bool Projector::toScreen(Vec3 v, Point& out) const {
    if (v.z > -kNear) return false;
    const Vec4 clip = proj * Vec4{v.x, v.y, v.z, 1.0};  // eye -> clip
    const double nx = clip.x / clip.w, ny = clip.y / clip.w;   // perspective divide -> NDC
    out = {(nx + 1) * 0.5 * width, (1 - ny) * 0.5 * height};  // viewport
    return true;
}

Vec3 Projector::rayDir(double px, double py) const {
    const double t = std::tan(kFovY / 2);
    const double aspect = static_cast<double>(width) / height;
    const double nx = (px + 0.5) / width * 2 - 1;
    const double ny = 1 - (py + 0.5) / height * 2;
    return normalize(pose.forward() + pose.right() * (nx * t * aspect) + pose.up() * (ny * t));
}

// ================================================================ sky
void drawSky(Canvas& c, const Projector& pr) {
    const int W = c.width(), H = c.height();
    // The nebula is smooth, so it is evaluated on a half-resolution grid and
    // bilinearly interpolated.
    const int gw = W / 2 + 2, gh = H / 2 + 2;
    std::vector<Color> grid(static_cast<std::size_t>(gw) * gh);
    for (int j = 0; j < gh; ++j)
        for (int i = 0; i < gw; ++i) grid[static_cast<std::size_t>(j) * gw + i] = skyColor(pr.rayDir(i * 2.0, j * 2.0));
    for (int y = 0; y < H; ++y) {
        const int j = y / 2;
        const double fy = (y % 2) * 0.5;
        for (int x = 0; x < W; ++x) {
            const int i = x / 2;
            const double fx = (x % 2) * 0.5;
            const Color a = grid[static_cast<std::size_t>(j) * gw + i], b = grid[static_cast<std::size_t>(j) * gw + i + 1];
            const Color d = grid[static_cast<std::size_t>(j + 1) * gw + i], e = grid[static_cast<std::size_t>(j + 1) * gw + i + 1];
            c.setPixel(x, y, mix(mix(a, b, fx), mix(d, e, fx), fy));
        }
    }

    // Stars are points at infinity: only the rotation part of the view matrix applies.
    for (const Star& s : starCatalog()) {
        const Vec3 v = transformDirection(pr.view, s.dir);
        if (v.z >= -0.01) continue;
        const double px = W / 2.0 + pr.focalPx * v.x / -v.z;
        const double py = H / 2.0 - pr.focalPx * v.y / -v.z;
        const int x = static_cast<int>(px), y = static_cast<int>(py);
        if (x < 0 || y < 0 || x >= W || y >= H) continue;
        c.blendPixel(x, y, s.tint, s.brightness);
        if (s.brightness > 0.72) {  // bright star: four short fading spikes (Bresenham)
            const int len = 2 + static_cast<int>(s.brightness * 4);
            const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& d : dirs)
                bresenhamLine(x, y, x + d[0] * len, y + d[1] * len, [&](int qx, int qy, int k) {
                    if (k > 0) c.addPixel(qx, qy, s.tint, 0.5 * (1.0 - static_cast<double>(k) / (len + 1)));
                });
        }
    }
}

// ================================================================ bodies
namespace {

struct ScreenSphere {
    bool visible = false;
    Point center;
    double radiusPx = 0;
    double depth = 0;  // distance along the view direction
};

ScreenSphere projectSphere(const Projector& pr, Vec3 world, double R) {
    ScreenSphere s;
    const Vec3 v = pr.toView(world);
    s.depth = -v.z;
    if (s.depth <= R * 0.5 + kNear) return s;
    if (!pr.toScreen(v, s.center)) return s;
    const double d2 = dot(v, v);
    s.radiusPx = pr.focalPx * R / std::sqrt(std::max(d2 - R * R, 1e-9));
    const double reach = s.radiusPx * 3.5 + 4;
    s.visible = s.center.x > -reach && s.center.y > -reach && s.center.x < pr.width + reach &&
                s.center.y < pr.height + reach;
    return s;
}

// Basis for shading a sphere seen from the eye: `a` points from the centre to
// the eye, `bx`/`by` span the screen-aligned plane. A pixel at (dx, dy) from
// the centre maps to the normal bx*dx + by*(-dy) + a*nz.
struct SphereBasis {
    Vec3 a, bx, by;
};

SphereBasis sphereBasis(const Projector& pr, Vec3 centre) {
    SphereBasis b;
    b.a = normalize(pr.pose.eye - centre);
    const Vec3 r = pr.pose.right();
    b.bx = normalize(r - b.a * dot(r, b.a));
    b.by = cross(b.a, b.bx);
    return b;
}

void drawSunBody(Canvas& c, const Projector& pr, const Galaxis& g, const ScreenSphere& ss) {
    const BodyState s = g.body(kSunIndex);
    const int cx = iround(ss.center.x), cy = iround(ss.center.y);
    const double R = ss.radiusPx;
    const int G = static_cast<int>(R * 3.4);

    circleSpans(cx, cy, G, [&](int y, int xl, int xr) {  // additive corona glow
        for (int x = xl; x <= xr; ++x) {
            const double d = std::hypot(x - ss.center.x, y - ss.center.y);
            const double t = std::max(0.0, d - R);
            double k = 0.85 * std::exp(-t / (0.35 * R)) + 0.22 * std::exp(-t / (1.3 * R));
            k *= clamp01((G - d) / (0.6 * R));
            c.addPixel(x, y, {255, 150, 60}, k);
        }
    });

    const SphereBasis B = sphereBasis(pr, s.position);
    const Mat4 toObject = transpose(s.orientation);
    const double rr = R + 0.5;
    circleSpans(cx, cy, static_cast<int>(std::ceil(R)), [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) {
            const double dx = x - ss.center.x, dy = y - ss.center.y;
            const double d2 = dx * dx + dy * dy;
            const double cover = clamp01(rr - std::sqrt(d2));
            if (cover <= 0) continue;
            const double nz = std::sqrt(std::max(0.0, rr * rr - d2)) / rr;
            const Vec3 N = B.bx * (dx / rr) + B.by * (-dy / rr) + B.a * nz;
            const Vec3 o = transformDirection(toObject, N);
            const Color base = sunSurfaceColor(std::atan2(o.x, o.z), std::asin(std::clamp(o.y, -1.0, 1.0)));
            const double t = std::sqrt(d2) / rr;
            const Color hot = mix({255, 246, 215}, base, 0.25 + 0.75 * std::pow(t, 1.2));
            const double limb = 0.55 + 0.45 * std::sqrt(nz);  // limb darkening
            c.blendPixel(x, y, scale(hot, limb * 1.05), cover);
        }
    });
}

// Saturn's rings, found per pixel by intersecting the view ray with the ring
// plane. `front` selects the part nearer than the planet's centre.
void drawRingsPass(Canvas& c, const Projector& pr, const BodyState& s, const ScreenSphere& ss, bool front) {
    const double reach = ss.radiusPx * kRingOuter * 1.15 + 2;
    const int x0 = std::max(0, static_cast<int>(ss.center.x - reach));
    const int x1 = std::min(pr.width - 1, static_cast<int>(ss.center.x + reach));
    const int y0 = std::max(0, static_cast<int>(ss.center.y - reach));
    const int y1 = std::min(pr.height - 1, static_cast<int>(ss.center.y + reach));
    const Vec3 eye = pr.pose.eye, fwd = pr.pose.forward();
    const Vec3 n = s.axis;
    const Vec3 L = normalize(-s.position);
    const double centreDepth = dot(s.position - eye, fwd);
    const double litSide = 0.45 + 0.55 * std::sqrt(std::abs(dot(n, L)));

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const Vec3 dir = pr.rayDir(x, y);
            const double denom = dot(dir, n);
            if (std::abs(denom) < 1e-6) continue;
            const double t = dot(s.position - eye, n) / denom;
            if (t <= 0) continue;
            const Vec3 h = eye + dir * t;
            const double rho = length(h - s.position) / s.radius;
            const double alpha = ringAlpha(rho);
            if (alpha <= 0) continue;
            if ((dot(h - eye, fwd) < centreDepth) != front) continue;
            // Planet's shadow on the rings.
            const Vec3 oc = s.position - h;
            const double along = dot(oc, L);
            const bool shadow = along > 0 && dot(oc, oc) - along * along < s.radius * s.radius;
            c.blendPixel(x, y, scale(ringColor(rho), litSide * (shadow ? 0.22 : 1.0)), alpha);
        }
    }
}

void drawPlanetBody(Canvas& c, const Projector& pr, const Galaxis& g, int i, const ScreenSphere& ss) {
    const PlanetInfo& p = solarSystem()[static_cast<std::size_t>(i)];
    const BodyState s = g.body(i);
    if (p.rings) drawRingsPass(c, pr, s, ss, false);

    const int cx = iround(ss.center.x), cy = iround(ss.center.y);
    const double rr = std::max(ss.radiusPx, 0.6);
    const SphereBasis B = sphereBasis(pr, s.position);
    const Mat4 toObject = transpose(s.orientation);
    const Vec3 L = normalize(-s.position);  // towards the Sun

    // Atmospheric glow just outside the limb.
    if (p.atmosphereStrength > 0 && rr >= 6) {
        const Vec3 Ls{dot(L, B.bx), -dot(L, B.by), 0};  // light direction in screen space
        circleSpans(cx, cy, static_cast<int>(rr * 1.12) + 2, [&](int y, int xl, int xr) {
            for (int x = xl; x <= xr; ++x) {
                const double dx = x - ss.center.x, dy = y - ss.center.y;
                const double d = std::hypot(dx, dy);
                if (d <= rr) continue;
                const double daySide = clamp01((dx * Ls.x + dy * Ls.y) / d + 0.35);
                c.addPixel(x, y, p.atmosphere, p.atmosphereStrength * 0.6 * std::exp(-(d - rr) / (0.035 * rr)) * daySide);
            }
        });
    }

    const double rpad = rr + 0.5;
    circleSpans(cx, cy, static_cast<int>(std::ceil(rr)), [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) {
            const double dx = x - ss.center.x, dy = y - ss.center.y;
            const double d2 = dx * dx + dy * dy;
            const double cover = clamp01(rpad - std::sqrt(d2));
            if (cover <= 0) continue;
            const double nz = std::sqrt(std::max(0.0, rpad * rpad - d2)) / rpad;
            const Vec3 N = B.bx * (dx / rpad) + B.by * (-dy / rpad) + B.a * nz;  // world-space normal
            const Vec3 o = transformDirection(toObject, N);                     // planet's own frame
            const Color surf = surfaceColor(p, std::atan2(o.x, o.z), std::asin(std::clamp(o.y, -1.0, 1.0)));

            const double lambert = dot(N, L);
            const double lit = 0.30 * smoothstep(-0.15, 0.25, lambert) + 0.70 * clamp01(lambert);
            Color col = scale(surf, 0.07 + 0.93 * lit);
            if (p.atmosphereStrength > 0) {
                const double rim = std::pow(1.0 - nz, 2.2);
                col = add(col, scale(p.atmosphere, rim * p.atmosphereStrength * clamp01(lambert + 0.3)));
            }
            if (p.rings) {  // the rings' shadow on the planet
                const Vec3 hit = s.position + N * s.radius;
                const double dn = dot(L, s.axis);
                if (std::abs(dn) > 1e-4) {
                    const double t = dot(s.position - hit, s.axis) / dn;
                    if (t > 0) {
                        const double rho = length(hit + L * t - s.position) / s.radius;
                        col = scale(col, 1.0 - 0.6 * ringAlpha(rho));
                    }
                }
            }
            c.blendPixel(x, y, col, cover);
        }
    });
    if (p.rings) drawRingsPass(c, pr, s, ss, true);
}

}  // namespace

void drawBody(Canvas& c, const Projector& pr, const Galaxis& g, int body) {
    const BodyState s = g.body(body);
    const ScreenSphere ss = projectSphere(pr, s.position, s.radius);
    if (!ss.visible) return;
    if (body == kSunIndex)
        drawSunBody(c, pr, g, ss);
    else
        drawPlanetBody(c, pr, g, body, ss);
}

// ================================================================ scene
namespace {

struct Item {
    double depth;
    int body;     // >= 0: a body; -1: a line segment
    Vec3 a, b;    // view-space segment
    Color col;
    double alpha;
};

void pushSegment(std::vector<Item>& items, Vec3 a, Vec3 b, Color col, double alpha) {
    // Clip against the near plane (z = -near) in view space.
    const double zn = -kNear * 4;
    if (a.z > zn && b.z > zn) return;
    if (a.z > zn) a = lerp(a, b, (zn - a.z) / (b.z - a.z));
    if (b.z > zn) b = lerp(b, a, (zn - b.z) / (a.z - b.z));
    items.push_back({-(a.z + b.z) / 2, -1, a, b, col, alpha});
}

void drawSegment(Canvas& c, const Projector& pr, const Item& it) {
    Point p0, p1;
    if (!pr.toScreen(it.a, p0) || !pr.toScreen(it.b, p1)) return;
    double x0 = p0.x, y0 = p0.y, x1 = p1.x, y1 = p1.y;
    const ClipRect screen{0, 0, pr.width - 1.0, pr.height - 1.0};
    if (!cohenSutherlandClip(x0, y0, x1, y1, screen)) return;
    bresenhamLine(iround(x0), iround(y0), iround(x1), iround(y1), [&](int x, int y, int k) {
        if (k > 0) c.blendPixel(x, y, it.col, it.alpha);  // skip the shared joint pixel
    });
}

}  // namespace

void renderWorld(Canvas& c, const Galaxis& g, const RenderOptions& opt) {
    const Projector pr(g.pose(), kFovY, c.width(), c.height());
    drawSky(c, pr);

    std::vector<Item> items;
    const auto& sys = solarSystem();
    const double sunDepth = std::max(1.0, -pr.toView({0, 0, 0}).z);
    if (g.showOrbits) {
        for (int i = 0; i < static_cast<int>(sys.size()); ++i) {
            const PlanetInfo& p = sys[static_cast<std::size_t>(i)];
            const double r = orbitRadiusWorld(p.orbitAU);
            const int n = 240;
            for (int k = 0; k < n; ++k) {
                const double a0 = 2 * kPi * k / n, a1 = 2 * kPi * (k + 1) / n;
                const Vec3 v0 = pr.toView({r * std::cos(a0), 0, r * std::sin(a0)});
                const Vec3 v1 = pr.toView({r * std::cos(a1), 0, r * std::sin(a1)});
                const double depth = std::max(0.1, -(v0.z + v1.z) / 2);
                const double alpha = 0.40 * std::clamp(std::pow(sunDepth / depth, 0.7), 0.55, 1.15);
                pushSegment(items, v0, v1, colors::Orbit, alpha);
            }
            // Fading trail behind the planet (it moves toward smaller angles).
            const double a = orbitAngle(i, g.day);
            const int tn = 40;
            const double arc = 0.6;
            for (int k = 0; k < tn; ++k) {
                const double a0 = a + arc * k / tn, a1 = a + arc * (k + 1) / tn;
                const Vec3 v0 = pr.toView({r * std::cos(a0), 0, r * std::sin(a0)});
                const Vec3 v1 = pr.toView({r * std::cos(a1), 0, r * std::sin(a1)});
                pushSegment(items, v0, v1, mix(p.base, colors::White, 0.35),
                            0.65 * std::pow(1.0 - static_cast<double>(k) / tn, 1.4));
            }
        }
    }
    for (int b = 0; b < kBodyCount; ++b) {
        if (b == g.focus() && g.focusAmount() > 0) continue;  // drawn last, on top of the dimming
        items.push_back({-pr.toView(g.body(b).position).z, b, {}, {}, {}, 0});
    }

    // Painter's algorithm: farthest first.
    std::stable_sort(items.begin(), items.end(), [](const Item& x, const Item& y) { return x.depth > y.depth; });
    for (const Item& it : items) {
        if (it.body >= 0)
            drawBody(c, pr, g, it.body);
        else
            drawSegment(c, pr, it);
    }

    const double f = g.focusAmount();
    if (g.focus() >= 0 && f > 0) {
        for (int y = 0; y < c.height(); ++y)
            for (int x = 0; x < c.width(); ++x) c.blendPixel(x, y, {2, 3, 9}, 0.90 * f);
        drawBody(c, pr, g, g.focus());
        if (opt.drawPanel) {
            const int pw = 480, ph = 560;
            const double a = g.panelAlpha();
            const int slide = iround((1.0 - a) * 48);
            drawFactPanel(c, c.width() - pw - 60 + slide, (c.height() - ph) / 2, pw, ph, factSheet(g.focus()), a);
        }
    }
}

}  // namespace galaxis
