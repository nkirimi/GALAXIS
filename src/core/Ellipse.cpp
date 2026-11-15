// Ellipse.cpp - midpoint ellipse algorithm and ellipse filling.
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "galaxis/Raster.h"

namespace galaxis {

namespace {

// Generates the first-quadrant points (x >= 0, y >= 0) of an ellipse centred
// at the origin, in order from (0, ry) to (rx, 0).
void ellipseQuadrant(int rx, int ry, const PlotFn& q) {
    const double rx2 = static_cast<double>(rx) * rx;
    const double ry2 = static_cast<double>(ry) * ry;
    int x = 0, y = ry;
    double px = 0.0;                // 2 * ry^2 * x
    double py = 2.0 * rx2 * y;      // 2 * rx^2 * y

    // Region 1: |slope| < 1, step in x every iteration.
    double p1 = ry2 - rx2 * ry + 0.25 * rx2;
    while (px < py) {
        q(x, y);
        ++x;
        px += 2.0 * ry2;
        if (p1 < 0) {
            p1 += ry2 + px;
        } else {
            --y;
            py -= 2.0 * rx2;
            p1 += ry2 + px - py;
        }
    }

    // Region 2: |slope| >= 1, step in y every iteration.
    double p2 = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1.0) * (y - 1.0) - rx2 * ry2;
    while (y >= 0) {
        q(x, y);
        --y;
        py -= 2.0 * rx2;
        if (p2 > 0) {
            p2 += rx2 - py;
        } else {
            ++x;
            px += 2.0 * ry2;
            p2 += rx2 - py + px;
        }
    }
}

}  // namespace

void midpointEllipse(int cx, int cy, int rx, int ry, const PlotFn& plot, Half half) {
    if (rx < 0 || ry < 0) return;
    const bool wantTop = half != Half::Bottom;
    const bool wantBottom = half != Half::Top;

    if (ry == 0) {  // degenerate: horizontal segment
        for (int x = -rx; x <= rx; ++x) plot(cx + x, cy);
        return;
    }
    if (rx == 0) {  // degenerate: vertical segment
        for (int y = -ry; y <= ry; ++y)
            if ((y <= 0 && wantTop) || (y > 0 && wantBottom)) plot(cx, cy + y);
        return;
    }

    ellipseQuadrant(rx, ry, [&](int x, int y) {
        auto pair = [&](int yy) {
            plot(cx + x, yy);
            if (x != 0) plot(cx - x, yy);  // avoid plotting the same pixel twice
        };
        if (y == 0) {
            pair(cy);
            return;
        }
        if (wantBottom) pair(cy + y);
        if (wantTop) pair(cy - y);
    });
}

void ellipseSpans(int cx, int cy, int rx, int ry, const SpanFn& span, Half half) {
    if (rx < 0 || ry < 0) return;
    std::vector<int> halfWidth(static_cast<std::size_t>(ry) + 1, -1);
    if (ry == 0) {
        halfWidth[0] = rx;
    } else {
        ellipseQuadrant(rx, ry, [&](int x, int y) {
            if (y >= 0 && y <= ry) halfWidth[y] = std::max(halfWidth[y], x);
        });
    }
    for (int dy = 0; dy <= ry; ++dy) {
        if (halfWidth[dy] < 0) continue;
        const int xl = cx - halfWidth[dy], xr = cx + halfWidth[dy];
        if (dy == 0) {
            if (half != Half::Bottom) span(cy, xl, xr);
            continue;
        }
        if (half != Half::Top) span(cy + dy, xl, xr);
        if (half != Half::Bottom) span(cy - dy, xl, xr);
    }
}

void drawEllipse(Canvas& c, int cx, int cy, int rx, int ry, Color col, double alpha, Half half) {
    midpointEllipse(cx, cy, rx, ry, [&](int x, int y) { c.blendPixel(x, y, col, alpha); }, half);
}

void fillEllipse(Canvas& c, int cx, int cy, int rx, int ry, Color col, double alpha, Half half) {
    ellipseSpans(cx, cy, rx, ry, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) c.blendPixel(x, y, col, alpha);
    }, half);
}

}  // namespace galaxis
