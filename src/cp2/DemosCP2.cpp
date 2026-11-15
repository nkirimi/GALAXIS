// DemosCP2.cpp - CP2 test sheets: 2-D transformations (Mat3) and 3-D
// transformations + projections (Mat4), drawn with the CP1 rasterisers.
#include "galaxis/DemosCP2.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "galaxis/Components.h"
#include "galaxis/Font.h"
#include "galaxis/Raster.h"
#include "galaxis/Renderer3D.h"
#include "galaxis/Texture.h"
#include "galaxis/Transform.h"
#include "galaxis/World.h"

namespace galaxis {

namespace {

const Color kSheetBg{8, 11, 22};
const Color kGrid{24, 34, 60};
const Color kAxis{70, 95, 150};
const Color kGhost{120, 135, 170};
const Color kShape{255, 190, 70};

int iround(double v) { return static_cast<int>(std::lround(v)); }

void sheetTitle(Canvas& c, const std::string& title, const std::string& subtitle) {
    drawText(c, 20, 16, title, colors::HudText, 2);
    drawText(c, 20, 38, subtitle, colors::HudDim, 1);
    drawLine(c, 20, 50, c.width() - 20, 50, colors::HudLine, 0.35);
}

std::string fmt(double v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%6.2f", std::abs(v) < 0.005 ? 0.0 : v);
    return buf;
}

void drawMatrixText(Canvas& c, int x, int y, const Mat3& m) {
    for (int r = 0; r < 3; ++r) {
        std::string line = "|";
        for (int k = 0; k < 3; ++k) line += fmt(m.at(r, k)) + " ";
        line += "|";
        drawText(c, x, y + r * 11, line, colors::HudText, 1);
    }
}

// --------------------------------------------------------------- 2-D sheet
// A panel shows a y-up coordinate system centred on (ox, oy).
struct Panel2D {
    int x, y, w, h;
    Point origin() const { return {x + w / 2.0 - 40, y + h / 2.0 + 30}; }
    Point toScreen(Point p) const { return {origin().x + p.x, origin().y - p.y}; }
    std::vector<Point> toScreen(const std::vector<Point>& poly) const {
        std::vector<Point> out;
        for (const Point& p : poly) out.push_back(toScreen(p));
        return out;
    }
};

void drawPanelFrame(Canvas& c, const Panel2D& p, const std::string& title) {
    c.setClip(p.x, p.y, p.x + p.w - 1, p.y + p.h - 1);
    fillPolygon(c, {{double(p.x), double(p.y)}, {double(p.x + p.w), double(p.y)}, {double(p.x + p.w), double(p.y + p.h)},
                    {double(p.x), double(p.y + p.h)}},
                {11, 16, 32});
    const Point o = p.origin();
    for (int gx = -200; gx <= 200; gx += 20) {
        drawLineDDA(c, iround(o.x + gx), p.y, iround(o.x + gx), p.y + p.h, kGrid);
        drawLineDDA(c, p.x, iround(o.y + gx), p.x + p.w, iround(o.y + gx), kGrid);
    }
    drawLine(c, p.x, iround(o.y), p.x + p.w, iround(o.y), kAxis);
    drawLine(c, iround(o.x), p.y, iround(o.x), p.y + p.h, kAxis);
    c.resetClip();
    drawPolygon(c, {{double(p.x), double(p.y)}, {double(p.x + p.w - 1), double(p.y)},
                    {double(p.x + p.w - 1), double(p.y + p.h - 1)}, {double(p.x), double(p.y + p.h - 1)}},
                colors::HudLine, 0.5);
    drawText(c, p.x, p.y - 14, title, colors::White, 1);
}

void drawShapePair(Canvas& c, const Panel2D& p, const std::vector<Point>& original, const Mat3& m) {
    c.setClip(p.x + 1, p.y + 1, p.x + p.w - 2, p.y + p.h - 2);
    const std::vector<Point> before = p.toScreen(original);
    const std::vector<Point> after = p.toScreen(transform(m, original));
    drawPolygon(c, before, kGhost, 0.6);
    fillPolygon(c, after, kShape, 0.85);
    drawPolygon(c, after, colors::White);
    c.resetClip();
}

void drawPivot(Canvas& c, const Panel2D& p, Point pivot) {
    const Point s = p.toScreen(pivot);
    fillCircle(c, iround(s.x), iround(s.y), 3, {255, 90, 90});
    drawCircle(c, iround(s.x), iround(s.y), 6, {255, 90, 90});
}

}  // namespace

Canvas demoTransform2D() {
    Canvas c(1280, 680, kSheetBg);
    sheetTitle(c, "07  2-D TRANSFORMATIONS (3X3 HOMOGENEOUS MATRICES)",
               "GREY = ORIGINAL SHAPE, GOLD = M * SHAPE.  COMPOSITES MULTIPLY RIGHT TO LEFT: T(P) * R * T(-P) ROTATES ABOUT P.");

    // An asymmetric "rocket" so reflections and rotations are easy to read.
    std::vector<Point> rocket = {{-10, -8}, {30, -8}, {44, 0}, {30, 8}, {6, 8}, {-2, 20}, {-10, 20}, {-6, 8}, {-10, 8}};
    rocket = transform(translate2D(20, 12), rocket);

    const int pw = 296, ph = 222, gap = 14, left = 22, top1 = 82, top2 = 388;
    auto panel = [&](int col, int row) { return Panel2D{left + col * (pw + gap), row == 0 ? top1 : top2, pw, ph}; };

    struct Case {
        std::string title;
        Mat3 m;
        bool pivot;
        Point p;
    };
    const Point pivot{50, 20};
    const std::vector<Case> cases = {
        {"TRANSLATE  T(60, 40)", translate2D(60, 40), false, {}},
        {"ROTATE 60 DEG ABOUT ORIGIN", rotate2D(radians(60)), false, {}},
        {"SCALE  S(1.6, 0.7)", scale2D(1.6, 0.7), false, {}},
        {"REFLECT IN THE X-AXIS", reflectX(), false, {}},
        {"SHEAR  SHX = 0.8", shear2D(0.8, 0), false, {}},
        {"ROTATE 90 DEG ABOUT PIVOT", rotateAbout(pivot, radians(90)), true, pivot},
        {"SCALE 1.8 ABOUT PIVOT", scaleAbout(pivot, 1.8, 1.8), true, pivot},
    };
    for (int i = 0; i < static_cast<int>(cases.size()); ++i) {
        const Panel2D p = panel(i % 4, i / 4);
        drawPanelFrame(c, p, cases[static_cast<std::size_t>(i)].title);
        drawShapePair(c, p, rocket, cases[static_cast<std::size_t>(i)].m);
        if (cases[static_cast<std::size_t>(i)].pivot) drawPivot(c, p, cases[static_cast<std::size_t>(i)].p);
        drawMatrixText(c, p.x, p.y + p.h + 8, cases[static_cast<std::size_t>(i)].m);
    }

    // Panel 8: how Galaxis places a planet. Its position is R(angle) * T(r, 0)
    // applied to the origin, re-evaluated every frame as the angle changes.
    const Panel2D p = panel(3, 1);
    drawPanelFrame(c, p, "GALAXIS ORBIT:  R(A) * T(R, 0)");
    c.setClip(p.x + 1, p.y + 1, p.x + p.w - 2, p.y + p.h - 2);
    const Point sun = p.toScreen({0, 0});
    drawCircle(c, iround(sun.x), iround(sun.y), 80, colors::Orbit, 0.8);
    fillCircle(c, iround(sun.x), iround(sun.y), 9, {255, 200, 90});
    for (int k = 0; k < 8; ++k) {
        const double a = radians(20 + k * 22.0);
        const Point pos = p.toScreen(rotate2D(a) * translate2D(80, 0) * Point{0, 0});
        const double alpha = 0.25 + 0.75 * k / 7.0;
        fillCircle(c, iround(pos.x), iround(pos.y), 6, {90, 160, 255}, alpha);
    }
    c.resetClip();
    drawMatrixText(c, p.x, p.y + p.h + 8, rotate2D(radians(174)) * translate2D(80, 0));
    return c;
}

// --------------------------------------------------------------- 3-D sheet
namespace {

struct Viewport3D {
    int x, y, w, h;
};

// Projects a world point through `m` (viewport * projection * view).
Point project(const Mat4& m, Vec3 p) {
    const Vec3 s = transformPoint(m, p);
    return {s.x, s.y};
}

Mat4 panelScreen(const Viewport3D& v) { return translate3D({double(v.x), double(v.y), 0}) * viewport(v.w, v.h); }

void frame3D(Canvas& c, const Viewport3D& v, const std::string& title) {
    fillPolygon(c, {{double(v.x), double(v.y)}, {double(v.x + v.w), double(v.y)}, {double(v.x + v.w), double(v.y + v.h)},
                    {double(v.x), double(v.y + v.h)}},
                {11, 16, 32});
    drawPolygon(c, {{double(v.x), double(v.y)}, {double(v.x + v.w - 1), double(v.y)},
                    {double(v.x + v.w - 1), double(v.y + v.h - 1)}, {double(v.x), double(v.y + v.h - 1)}},
                colors::HudLine, 0.5);
    drawText(c, v.x, v.y - 14, title, colors::White, 1);
}

// Wireframe globe: latitude rings and meridians, back side dimmed.
void wireSphere(Canvas& c, const Viewport3D& v, const Mat4& model, const Mat4& viewProj) {
    const Mat4 screen = panelScreen(v) * viewProj;
    const Mat4 mv = model;
    auto seg = [&](Vec3 a, Vec3 b, Color col) {
        const Vec3 wa = transformPoint(mv, a), wb = transformPoint(mv, b);
        const bool back = (wa.z + wb.z) < 0;  // the orthographic camera looks down -z
        const Point pa = project(screen, wa), pb = project(screen, wb);
        drawLine(c, iround(pa.x), iround(pa.y), iround(pb.x), iround(pb.y), col, back ? 0.22 : 0.95);
    };
    const int n = 48;
    for (int lat = -60; lat <= 60; lat += 30)
        for (int k = 0; k < n; ++k) {
            const double a0 = 2 * kPi * k / n, a1 = 2 * kPi * (k + 1) / n, la = radians(lat);
            seg({std::cos(la) * std::cos(a0), std::sin(la), std::cos(la) * std::sin(a0)},
                {std::cos(la) * std::cos(a1), std::sin(la), std::cos(la) * std::sin(a1)}, lat == 0 ? kShape : Color{110, 160, 255});
        }
    for (int lon = 0; lon < 180; lon += 30)
        for (int k = 0; k < n; ++k) {
            const double t0 = 2 * kPi * k / n, t1 = 2 * kPi * (k + 1) / n, lo = radians(lon);
            seg({std::cos(t0) * std::cos(lo), std::sin(t0), std::cos(t0) * std::sin(lo)},
                {std::cos(t1) * std::cos(lo), std::sin(t1), std::cos(t1) * std::sin(lo)}, lon == 0 ? Color{255, 120, 120} : Color{110, 160, 255});
        }
    seg({0, -1.45, 0}, {0, 1.45, 0}, colors::White);  // spin axis
}

void drawMat4Text(Canvas& c, int x, int y, const Mat4& m) {
    for (int r = 0; r < 4; ++r) {
        std::string line = "|";
        for (int k = 0; k < 4; ++k) line += fmt(m.at(r, k)) + " ";
        line += "|";
        drawText(c, x, y + r * 11, line, colors::HudText, 1);
    }
}

void orbitsThrough(Canvas& c, const Viewport3D& v, const Mat4& view, const Mat4& proj, double day) {
    c.setClip(v.x + 1, v.y + 1, v.x + v.w - 2, v.y + v.h - 2);
    const Mat4 screen = panelScreen(v) * proj * view;
    const auto& sys = solarSystem();
    for (int i = 0; i < 8; ++i) {
        const double r = orbitRadiusWorld(sys[static_cast<std::size_t>(i)].orbitAU);
        const int n = 160;
        for (int k = 0; k < n; ++k) {
            const double a0 = 2 * kPi * k / n, a1 = 2 * kPi * (k + 1) / n;
            const Point p0 = project(screen, {r * std::cos(a0), 0, r * std::sin(a0)});
            const Point p1 = project(screen, {r * std::cos(a1), 0, r * std::sin(a1)});
            drawLine(c, iround(p0.x), iround(p0.y), iround(p1.x), iround(p1.y), colors::Orbit, 0.6);
        }
        const double a = orbitAngle(i, day);
        const Point p = project(screen, {r * std::cos(a), 0, r * std::sin(a)});
        fillCircle(c, iround(p.x), iround(p.y), i >= 4 ? 4 : 3, sys[static_cast<std::size_t>(i)].base);
    }
    const Point s = project(screen, {0, 0, 0});
    fillCircle(c, iround(s.x), iround(s.y), 6, {255, 200, 90});
    c.resetClip();
}

}  // namespace

Canvas demoTransform3D() {
    Canvas c(1280, 720, kSheetBg);
    sheetTitle(c, "08  3-D TRANSFORMATIONS AND PROJECTION (4X4 MATRICES)",
               "WORLD -> VIEW (LOOKAT) -> CLIP (PROJECTION) -> NDC (DIVIDE BY W) -> SCREEN (VIEWPORT).  RED MERIDIAN AND GOLD EQUATOR SHOW ORIENTATION.");

    // Row 1: a globe under rotations, viewed with an orthographic camera.
    const Mat4 orthoView = lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    const Mat4 orthoProj = orthographic(-1.6, 1.6, -1.6, 1.6, 0.1, 10);
    const Mat4 vp = orthoProj * orthoView;
    struct Case {
        std::string title;
        Mat4 m;
    };
    const std::vector<Case> cases = {
        {"ORIGINAL", Mat4::identity()},
        {"ROTATE X 30 DEG", rotateX(radians(30))},
        {"ROTATE Y 40 * ROTATE X 30", rotateY(radians(40)) * rotateX(radians(30))},
        {"URANUS: TILT 98 * SPIN 40", rotateX(radians(18)) * rotateZ(radians(-97.8)) * rotateY(radians(40))},
    };
    for (int i = 0; i < 4; ++i) {
        const Viewport3D v{22 + i * 312, 80, 230, 230};
        frame3D(c, v, cases[static_cast<std::size_t>(i)].title);
        wireSphere(c, v, cases[static_cast<std::size_t>(i)].m, vp);
        drawMat4Text(c, v.x, v.y + v.h + 8, cases[static_cast<std::size_t>(i)].m);
    }

    // Row 2: the same orbits under three different view + projection matrices.
    const double day = 120;
    const Viewport3D top{22, 420, 380, 280}, side{450, 420, 380, 280}, persp{878, 420, 380, 280};
    const double asp = 380.0 / 280.0;
    frame3D(c, top, "TOP VIEW: ORTHOGRAPHIC");
    orbitsThrough(c, top, lookAt({0, 100, 0}, {0, 0, 0}, {0, 0, -1}), orthographic(-46 * asp, 46 * asp, -46, 46, 1, 300), day);
    frame3D(c, side, "SIDE VIEW: ORTHOGRAPHIC (EDGE-ON)");
    orbitsThrough(c, side, lookAt({0, 0, 100}, {0, 0, 0}, {0, 1, 0}), orthographic(-46 * asp, 46 * asp, -46, 46, 1, 300), day);
    frame3D(c, persp, "GALAXIS CAMERA: PERSPECTIVE 30 DEG");
    Galaxis g;
    g.day = day;
    g.aspect = asp;
    g.camera.distance = 128;
    orbitsThrough(c, persp, g.view(), g.projection(), day);
    return c;
}

}  // namespace galaxis
