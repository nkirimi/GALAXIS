// Circle.cpp - midpoint circle algorithm and circle filling.
#include <algorithm>
#include <cstdlib>

#include "galaxis/Raster.h"

namespace galaxis {

// Octants are numbered clockwise starting at 12 o'clock:
//   0: (x,-y)  1: (y,-x)  2: (y,x)  3: (x,y)  4: (-x,y)  5: (-y,x)  6: (-y,-x)  7: (-x,-y)
// `step` is the iteration count inside the octant, the same for all eight
// mirrored points, which is what makes symmetric dash patterns trivial.
void midpointCircle(int cx, int cy, int r, const CirclePlotFn& plot) {
    if (r < 0) return;
    int x = 0, y = r;
    int p = 1 - r;  // decision parameter for the midpoint (1, r - 1/2)
    int step = 0;
    while (x <= y) {
        plot(cx + x, cy - y, step, 0);
        plot(cx + y, cy - x, step, 1);
        plot(cx + y, cy + x, step, 2);
        plot(cx + x, cy + y, step, 3);
        plot(cx - x, cy + y, step, 4);
        plot(cx - y, cy + x, step, 5);
        plot(cx - y, cy - x, step, 6);
        plot(cx - x, cy - y, step, 7);
        ++x;
        if (p < 0) {
            p += 2 * x + 1;            // midpoint inside: keep y
        } else {
            --y;
            p += 2 * (x - y) + 1;      // midpoint outside: step y inward
        }
        ++step;
    }
}

void circleSpans(int cx, int cy, int r, const SpanFn& span) {
    if (r < 0) return;
    std::vector<int> half(static_cast<std::size_t>(r) + 1, -1);
    midpointCircle(0, 0, r, [&](int x, int y, int, int) {
        const int ay = std::abs(y), ax = std::abs(x);
        if (ay <= r) half[ay] = std::max(half[ay], ax);
    });
    for (int dy = 0; dy <= r; ++dy) {
        if (half[dy] < 0) continue;
        span(cy + dy, cx - half[dy], cx + half[dy]);
        if (dy != 0) span(cy - dy, cx - half[dy], cx + half[dy]);
    }
}

void drawCircle(Canvas& c, int cx, int cy, int r, Color col, double alpha) {
    midpointCircle(cx, cy, r, [&](int x, int y, int, int) { c.blendPixel(x, y, col, alpha); });
}

void fillCircle(Canvas& c, int cx, int cy, int r, Color col, double alpha) {
    circleSpans(cx, cy, r, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) c.blendPixel(x, y, col, alpha);
    });
}

void drawDashedCircle(Canvas& c, int cx, int cy, int r, Color col, int dash, int gap, double alpha) {
    const int period = std::max(1, dash + gap);
    midpointCircle(cx, cy, r, [&](int x, int y, int step, int) {
        if (step % period < dash) c.blendPixel(x, y, col, alpha);
    });
}

}  // namespace galaxis
