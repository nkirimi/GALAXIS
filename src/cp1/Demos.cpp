// Demos.cpp - test sheets that show each algorithm on its own.
#include "galaxis/Demos.h"

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "galaxis/Clip.h"
#include "galaxis/Components.h"
#include "galaxis/Font.h"
#include "galaxis/Raster.h"

namespace galaxis {

namespace {

const Color kSheetBg{8, 11, 22};

void sheetTitle(Canvas& c, const std::string& title, const std::string& subtitle) {
    drawText(c, 20, 16, title, colors::HudText, 2);
    drawText(c, 20, 38, subtitle, colors::HudDim, 1);
    drawLine(c, 20, 50, c.width() - 20, 50, colors::HudLine, 0.35);
}

void centeredLabel(Canvas& c, int cx, int y, const std::string& text, int scale = 1, Color col = colors::HudText) {
    drawText(c, cx - textWidth(text, scale) / 2, y, text, col, scale);
}

// Draws an enlarged pixel grid of a short line so individual pixel choices
// of the algorithm are visible, with the ideal line overlaid in red.
void zoomInset(Canvas& c, int ox, int oy, bool useDDA) {
    const int cell = 8, gw = 24, gh = 9;
    const int x1 = 23, y1 = 8;
    for (int gx = 0; gx <= gw; ++gx) drawLine(c, ox + gx * cell, oy, ox + gx * cell, oy + gh * cell, colors::HudLine, 0.25);
    for (int gy = 0; gy <= gh; ++gy) drawLine(c, ox, oy + gy * cell, ox + gw * cell, oy + gy * cell, colors::HudLine, 0.25);

    auto plotCell = [&](int x, int y, int) {
        fillPolygon(c, {{double(ox + x * cell + 1), double(oy + y * cell + 1)},
                        {double(ox + (x + 1) * cell), double(oy + y * cell + 1)},
                        {double(ox + (x + 1) * cell), double(oy + (y + 1) * cell)},
                        {double(ox + x * cell + 1), double(oy + (y + 1) * cell)}},
                    useDDA ? Color{110, 200, 255} : Color{255, 190, 70});
    };
    if (useDDA)
        ddaLine(0, 0, x1, y1, plotCell);
    else
        bresenhamLine(0, 0, x1, y1, plotCell);

    drawLine(c, ox + cell / 2, oy + cell / 2, ox + x1 * cell + cell / 2, oy + y1 * cell + cell / 2, {255, 80, 80});
}

}  // namespace

Canvas demoLines() {
    Canvas c(960, 470, kSheetBg);
    sheetTitle(c, "01  LINE DRAWING", "32 LINES IN EVERY DIRECTION (ALL 8 OCTANTS) + 8X ZOOM OF THE LINE (0,0)-(23,8)");
    drawLine(c, 480, 60, 480, 455, colors::HudLine, 0.35);

    for (int panel = 0; panel < 2; ++panel) {
        const int ox = panel * 480;
        const int cx = ox + 240, cy = 205;
        for (int k = 0; k < 32; ++k) {
            const double a = 2 * kPi * k / 32;
            const int ex = static_cast<int>(std::lround(cx + 140 * std::cos(a)));
            const int ey = static_cast<int>(std::lround(cy + 140 * std::sin(a)));
            const Color col = hsv(static_cast<double>(k) / 32, 0.55, 1.0);
            if (panel == 0)
                drawLineDDA(c, cx, cy, ex, ey, col);
            else
                drawLine(c, cx, cy, ex, ey, col);
        }
        drawText(c, ox + 20, 62, panel == 0 ? "DDA" : "BRESENHAM", colors::White, 2);
        drawText(c, ox + 20, 82, panel == 0 ? "FLOATING POINT + ROUNDING" : "INTEGER DECISION PARAMETER", colors::HudDim, 1);
        zoomInset(c, ox + 144, 370, panel == 0);
    }
    return c;
}

Canvas demoCircles() {
    Canvas c(1000, 400, kSheetBg);
    sheetTitle(c, "02  MIDPOINT CIRCLE", "ONE OCTANT IS COMPUTED, THE OTHER SEVEN ARE MIRRORED. FILLS USE ONE SPAN PER SCANLINE.");
    const int cy = 210;

    // (a) concentric outlines
    for (int r = 15; r <= 105; r += 15) drawCircle(c, 125, cy, r, hsv(r / 140.0, 0.5, 1.0));
    centeredLabel(c, 125, 340, "OUTLINES", 2);

    // (b) 8-way symmetry: colour each mirrored octant differently, plus a dashed ring
    midpointCircle(375, cy, 100, [&](int x, int y, int, int oct) { c.setPixel(x, y, hsv(oct / 8.0, 0.65, 1.0)); });
    drawDashedCircle(c, 375, cy, 70, colors::Accent, 5, 4);
    for (int k = 0; k < 8; ++k) {
        const double a = -kPi / 2 + kPi / 8 + k * kPi / 4;  // middle of octant k
        drawText(c, static_cast<int>(375 + 85 * std::cos(a)) - 2, static_cast<int>(cy + 85 * std::sin(a)) - 3,
                 std::to_string(k), hsv(k / 8.0, 0.65, 1.0), 1);
    }
    centeredLabel(c, 375, 340, "8-WAY SYMMETRY", 2);

    // (c) filled via spans, alternate rows tinted to show the scanlines
    circleSpans(625, cy, 100, [&](int y, int xl, int xr) {
        const Color col = (y % 2 == 0) ? Color{80, 145, 240} : Color{35, 70, 150};
        for (int x = xl; x <= xr; ++x) c.setPixel(x, y, col);
    });
    drawCircle(c, 625, cy, 100, colors::White);
    centeredLabel(c, 625, 340, "SPAN FILL", 2);

    // (d) shaded sphere built on the same spans
    drawPlanet(c, solarSystem()[2], {875.0, double(cy)}, 100, {-0.6, -0.5, 0.62});
    centeredLabel(c, 875, 340, "SHADED SPHERE", 2);
    centeredLabel(c, 875, 362, "LAMBERT LIGHT = DAY / NIGHT", 1, colors::HudDim);
    return c;
}

Canvas demoEllipses() {
    Canvas c(1000, 420, kSheetBg);
    sheetTitle(c, "03  MIDPOINT ELLIPSE", "ORBITS SEEN FROM ABOVE BECOME ELLIPSES. FAR HALF IS DRAWN BEFORE THE SUN, NEAR HALF AFTER.");

    // (a) orbit stack, far halves dim, near halves bright
    const int ox = 300, oy = 235;
    for (int i = 0; i < 6; ++i) {
        const int rx = 60 + i * 44, ry = static_cast<int>(rx * 0.4);
        drawEllipse(c, ox, oy, rx, ry, colors::Orbit, 0.45, Half::Top);
    }
    drawSun(c, {double(ox), double(oy)}, 22);
    for (int i = 0; i < 6; ++i) {
        const int rx = 60 + i * 44, ry = static_cast<int>(rx * 0.4);
        drawEllipse(c, ox, oy, rx, ry, {150, 185, 255}, 1.0, Half::Bottom);
    }
    centeredLabel(c, ox, 92, "FAR HALF (BEHIND SUN, DIMMED)", 1, colors::HudDim);
    centeredLabel(c, ox, 375, "NEAR HALF (IN FRONT OF SUN)", 2);

    // (b) 4-way symmetry
    const int qx = 800, qy = 150;
    midpointEllipse(qx, qy, 150, 70, [&](int x, int y) {
        const int quad = (x >= qx ? 0 : 1) + (y >= qy ? 2 : 0);
        c.setPixel(x, y, hsv(quad / 4.0 + 0.05, 0.6, 1.0));
    });
    drawLine(c, qx - 160, qy, qx + 160, qy, colors::HudLine, 0.3);
    drawLine(c, qx, qy - 80, qx, qy + 80, colors::HudLine, 0.3);
    centeredLabel(c, qx, qy + 82, "4-WAY SYMMETRY", 2);

    // (c) span-filled ellipse + Saturn-style ring halves around a planet
    fillEllipse(c, qx, 320, 150, 40, {35, 55, 100});
    drawEllipse(c, qx, 320, 150, 40, colors::HudLine);
    const PlanetInfo& saturn = solarSystem()[5];
    drawRings(c, {double(qx), 320.0}, 30, 0.4, Half::Top);
    drawPlanet(c, saturn, {double(qx), 320.0}, 30, {-0.6, -0.4, 0.7});
    drawRings(c, {double(qx), 320.0}, 30, 0.4, Half::Bottom);
    centeredLabel(c, qx, 375, "FILLED ELLIPSE + RING HALVES", 2);
    return c;
}

Canvas demoFills() {
    Canvas c(1000, 400, kSheetBg);
    sheetTitle(c, "04  AREA FILLING", "SCANLINE (EDGE TABLE + ACTIVE EDGE TABLE), BOUNDARY FILL AND FLOOD FILL (4-CONNECTED, ITERATIVE)");
    const int cy = 200;

    // (a) concave star, scanline fill with a vertical gradient per span
    std::vector<Point> star;
    for (int k = 0; k < 10; ++k) {
        const double a = -kPi / 2 + k * kPi / 5;
        const double r = (k % 2 == 0) ? 100 : 42;
        star.push_back({125 + r * std::cos(a), cy + r * std::sin(a)});
    }
    scanlinePolygon(star, [&](int y, int xl, int xr) {
        const Color col = mix({255, 210, 90}, {255, 110, 50}, (y - (cy - 100)) / 200.0);
        for (int x = xl; x <= xr; ++x) c.setPixel(x, y, col);
    });
    drawPolygon(c, star, colors::White);
    centeredLabel(c, 125, 330, "SCANLINE (CONCAVE)", 2);

    // (b) HUD panel = chamfered polygon + gradient + outline
    drawPanel(c, 280, cy - 70, 190, 140, 16, 0.9);
    drawText(c, 300, cy - 45, "HUD PANEL", colors::White, 2);
    drawText(c, 300, cy - 20, "6-VERTEX POLYGON", colors::HudDim, 1);
    drawText(c, 300, cy - 6, "GRADIENT SPANS", colors::HudDim, 1);
    drawText(c, 300, cy + 8, "BLENDED 90%", colors::HudDim, 1);
    centeredLabel(c, 375, 330, "SCANLINE (PANEL)", 2);

    // (c) boundary fill: triangle outline then fill from an interior seed
    const Color edge{240, 240, 255}, fill{90, 200, 140};
    const std::vector<Point> tri = {{625, cy - 95.0}, {720, cy + 80.0}, {530, cy + 80.0}};
    drawPolygon(c, tri, edge);
    boundaryFill4(c, 625, cy + 20, fill, edge);
    fillCircle(c, 625, cy + 20, 3, {255, 80, 80});
    drawText(c, 633, cy + 16, "SEED", {255, 120, 120}, 1);
    centeredLabel(c, 625, 330, "BOUNDARY FILL", 2);

    // (d) flood fill: a circle cut by two lines -> fill one region only
    const Color outline{200, 210, 255};
    drawCircle(c, 875, cy, 95, outline);
    drawLine(c, 780, cy - 30, 970, cy + 40, outline);
    drawLine(c, 830, cy + 85, 905, cy - 92, outline);
    floodFill4(c, 920, cy + 50, {110, 150, 255});
    fillCircle(c, 920, cy + 50, 3, {255, 80, 80});
    centeredLabel(c, 875, 330, "FLOOD FILL (4)", 2);
    centeredLabel(c, 875, 352, "ONLY THE SEEDED REGION CHANGES", 1, colors::HudDim);
    centeredLabel(c, 875, 366, "8-CONNECTED WOULD LEAK THROUGH", 1, colors::HudDim);
    centeredLabel(c, 875, 378, "THE DIAGONAL STEPS OF THIN LINES", 1, colors::HudDim);
    return c;
}

Canvas demoClipping() {
    Canvas c(960, 450, kSheetBg);
    sheetTitle(c, "05  COHEN-SUTHERLAND CLIPPING", "RED = ORIGINAL SEGMENT, GREEN = PART KEPT AFTER CLIPPING. CODES ARE TOP-BOTTOM-RIGHT-LEFT.");
    const ClipRect win{300, 150, 660, 330};

    // region code labels in the 9 zones
    const int zx[3] = {150, 480, 810};
    const int zy[3] = {100, 240, 400};
    const char* codes[3][3] = {{"1001", "1000", "1010"}, {"0001", "0000", "0010"}, {"0101", "0100", "0110"}};
    for (int r = 0; r < 3; ++r)
        for (int col = 0; col < 3; ++col) centeredLabel(c, zx[col], zy[r] - 7, codes[r][col], 2, {90, 90, 60});

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> X(30, 930), Y(65, 435);
    for (int i = 0; i < 40; ++i) {
        double x0 = X(rng), y0 = Y(rng), x1 = X(rng), y1 = Y(rng);
        drawLine(c, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1), {220, 70, 70},
                 0.35);
        drawLineClipped(c, x0, y0, x1, y1, win, {90, 235, 120});
    }
    drawPolygon(c, {{win.xmin, win.ymin}, {win.xmax, win.ymin}, {win.xmax, win.ymax}, {win.xmin, win.ymax}},
                colors::White);
    drawText(c, static_cast<int>(win.xmin) + 6, static_cast<int>(win.ymin) + 6, "CLIP WINDOW", colors::White, 1);
    return c;
}

Canvas demoComponents() {
    Canvas c(1280, 330, kSheetBg);
    drawBackground(c, 99);
    sheetTitle(c, "06  GALAXIS COMPONENTS", "EVERY BODY IN THE SCENE, DRAWN ONLY WITH THE ALGORITHMS IN SHEETS 01-05");

    // Sun + planets row (lit from the Sun on the left)
    drawSun(c, {120.0, 200.0}, 44);
    centeredLabel(c, 120, 300, "SUN", 1);

    const auto& sys = solarSystem();
    const int xs[8] = {270, 340, 420, 495, 590, 740, 880, 985};
    const int rs[8] = {8, 14, 14, 10, 34, 28, 20, 20};
    for (int i = 0; i < 8; ++i) {
        const PlanetInfo& p = sys[static_cast<std::size_t>(i)];
        const Point pos{double(xs[i]), 200.0};
        const Vec3 light{-1.0, -0.15, 0.45};
        if (p.rings) drawRings(c, pos, rs[i], 0.4, Half::Top);
        drawAtmosphereHalo(c, p, pos, rs[i], light);
        drawPlanet(c, p, pos, rs[i], light);
        if (p.rings) drawRings(c, pos, rs[i], 0.4, Half::Bottom);
        centeredLabel(c, xs[i], 270, p.name, 1);
    }

    // Orbit arc with a fading trail
    const OrbitView v{{1130.0, 140.0}, 0.4};
    drawOrbit(c, v, 110, colors::Orbit, 0.6, Half::Both);
    drawTrail(c, v, 110, 5.75, 1.6, {200, 220, 255}, 0.95, Half::Both);
    const Point tp = orbitPoint(v, 110, 5.75);
    drawPlanet(c, sys[3], tp, 6, lightFromOrbit(5.75, 0.4));
    centeredLabel(c, 1130, 220, "ORBIT + TRAIL", 1);

    return c;
}

}  // namespace galaxis
