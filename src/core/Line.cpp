// Line.cpp - DDA and Bresenham line algorithms.
#include <cmath>
#include <cstdlib>

#include "galaxis/Raster.h"

namespace galaxis {

void ddaLine(int x0, int y0, int x1, int y1, const StepPlotFn& plot) {
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) {
        plot(x0, y0, 0);
        return;
    }
    const double xInc = static_cast<double>(dx) / steps;
    const double yInc = static_cast<double>(dy) / steps;
    double x = x0, y = y0;
    for (int k = 0; k <= steps; ++k) {
        plot(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), k);
        x += xInc;
        y += yInc;
    }
}

void bresenhamLine(int x0, int y0, int x1, int y1, const StepPlotFn& plot) {
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = (x1 >= x0) ? 1 : -1;
    const int sy = (y1 >= y0) ? 1 : -1;
    int x = x0, y = y0;

    if (dx >= dy) {
        // |slope| <= 1: x is the driving axis.
        int p = 2 * dy - dx;
        for (int k = 0; k <= dx; ++k) {
            plot(x, y, k);
            if (p >= 0) {
                y += sy;
                p += 2 * (dy - dx);
            } else {
                p += 2 * dy;
            }
            x += sx;
        }
    } else {
        // |slope| > 1: y is the driving axis (roles of x and y swapped).
        int p = 2 * dx - dy;
        for (int k = 0; k <= dy; ++k) {
            plot(x, y, k);
            if (p >= 0) {
                x += sx;
                p += 2 * (dx - dy);
            } else {
                p += 2 * dx;
            }
            y += sy;
        }
    }
}

void drawLineDDA(Canvas& c, int x0, int y0, int x1, int y1, Color col, double alpha) {
    ddaLine(x0, y0, x1, y1, [&](int x, int y, int) { c.blendPixel(x, y, col, alpha); });
}

void drawLine(Canvas& c, int x0, int y0, int x1, int y1, Color col, double alpha) {
    bresenhamLine(x0, y0, x1, y1, [&](int x, int y, int) { c.blendPixel(x, y, col, alpha); });
}

// A thick line is a Bresenham line whose "pen" is a small square. Pixels
// already covered by the previous pen position are skipped so that
// semi-transparent thick lines blend evenly.
void drawThickLine(Canvas& c, int x0, int y0, int x1, int y1, int thickness, Color col, double alpha) {
    const int lo = -(thickness - 1) / 2;
    const int hi = thickness / 2;
    int px = 0, py = 0;
    bool havePrev = false;
    bresenhamLine(x0, y0, x1, y1, [&](int x, int y, int) {
        for (int oy = lo; oy <= hi; ++oy) {
            for (int ox = lo; ox <= hi; ++ox) {
                const int qx = x + ox, qy = y + oy;
                if (havePrev && qx >= px + lo && qx <= px + hi && qy >= py + lo && qy <= py + hi) {
                    continue;  // inside the previous pen square
                }
                c.blendPixel(qx, qy, col, alpha);
            }
        }
        px = x;
        py = y;
        havePrev = true;
    });
}

}  // namespace galaxis
