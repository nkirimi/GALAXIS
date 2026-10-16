// Fill.cpp - scanline polygon fill, boundary fill and flood fill.
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "galaxis/Raster.h"

namespace galaxis {

void scanlinePolygon(const std::vector<Point>& poly, const SpanFn& span) {
    const std::size_t n = poly.size();
    if (n < 3) return;

    struct Edge {
        int yEnd;     // first scanline NOT covered by this edge
        double x;     // x where the edge crosses the current scanline centre
        double dxdy;  // inverse slope: change in x per scanline
    };

    double ymin = std::numeric_limits<double>::max();
    double ymax = std::numeric_limits<double>::lowest();
    for (const Point& p : poly) {
        ymin = std::min(ymin, p.y);
        ymax = std::max(ymax, p.y);
    }
    // Scanline y covers the pixel row whose centre is at y + 0.5.
    const int rowMin = static_cast<int>(std::ceil(ymin - 0.5));
    const int rowMax = static_cast<int>(std::ceil(ymax - 0.5));  // exclusive
    if (rowMax <= rowMin) return;

    // 1. Build the Edge Table, bucketed by the first scanline each edge touches.
    std::vector<std::vector<Edge>> edgeTable(static_cast<std::size_t>(rowMax - rowMin));
    for (std::size_t i = 0; i < n; ++i) {
        Point a = poly[i];
        Point b = poly[(i + 1) % n];
        if (a.y == b.y) continue;  // horizontal edges never cross a scanline centre
        if (a.y > b.y) std::swap(a, b);
        const int yStart = static_cast<int>(std::ceil(a.y - 0.5));
        const int yEnd = static_cast<int>(std::ceil(b.y - 0.5));
        if (yStart >= yEnd) continue;
        const double dxdy = (b.x - a.x) / (b.y - a.y);
        const double x = a.x + ((yStart + 0.5) - a.y) * dxdy;
        edgeTable[static_cast<std::size_t>(yStart - rowMin)].push_back({yEnd, x, dxdy});
    }

    // 2. Sweep scanlines, maintaining the Active Edge Table.
    std::vector<Edge> active;
    std::vector<double> xs;
    for (int y = rowMin; y < rowMax; ++y) {
        for (const Edge& e : edgeTable[static_cast<std::size_t>(y - rowMin)]) active.push_back(e);
        active.erase(std::remove_if(active.begin(), active.end(), [y](const Edge& e) { return e.yEnd <= y; }),
                     active.end());

        xs.clear();
        for (const Edge& e : active) xs.push_back(e.x);
        std::sort(xs.begin(), xs.end());

        // 3. Fill between pairs of intersections (even-odd rule).
        for (std::size_t i = 0; i + 1 < xs.size(); i += 2) {
            const int xl = static_cast<int>(std::ceil(xs[i] - 0.5));
            const int xr = static_cast<int>(std::ceil(xs[i + 1] - 0.5)) - 1;
            if (xl <= xr) span(y, xl, xr);
        }

        // 4. Incremental update: x(y+1) = x(y) + 1/m.
        for (Edge& e : active) e.x += e.dxdy;
    }
}

void fillPolygon(Canvas& c, const std::vector<Point>& poly, Color col, double alpha) {
    scanlinePolygon(poly, [&](int y, int xl, int xr) {
        for (int x = xl; x <= xr; ++x) c.blendPixel(x, y, col, alpha);
    });
}

void drawPolygon(Canvas& c, const std::vector<Point>& poly, Color col, double alpha) {
    const std::size_t n = poly.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Point a = poly[i];
        const Point b = poly[(i + 1) % n];
        // Skip step 0 so shared vertices are not blended twice.
        bresenhamLine(static_cast<int>(std::lround(a.x)), static_cast<int>(std::lround(a.y)),
                      static_cast<int>(std::lround(b.x)), static_cast<int>(std::lround(b.y)),
                      [&](int x, int y, int k) {
                          if (k > 0 || alpha >= 1.0) c.blendPixel(x, y, col, alpha);
                      });
    }
}

void boundaryFill4(Canvas& c, int x, int y, Color fill, Color boundary) {
    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(x, y);
    while (!stack.empty()) {
        const auto [px, py] = stack.back();
        stack.pop_back();
        if (!c.visible(px, py)) continue;
        const Color cur = c.getPixel(px, py);
        if (cur == boundary || cur == fill) continue;
        c.setPixel(px, py, fill);
        stack.emplace_back(px + 1, py);
        stack.emplace_back(px - 1, py);
        stack.emplace_back(px, py + 1);
        stack.emplace_back(px, py - 1);
    }
}

namespace {
void floodFill(Canvas& c, int x, int y, Color fill, bool eightConnected) {
    if (!c.visible(x, y)) return;
    const Color target = c.getPixel(x, y);
    if (target == fill) return;
    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(x, y);
    while (!stack.empty()) {
        const auto [px, py] = stack.back();
        stack.pop_back();
        if (!c.visible(px, py) || c.getPixel(px, py) != target) continue;
        c.setPixel(px, py, fill);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                if (!eightConnected && dx != 0 && dy != 0) continue;  // 4-connected: no diagonals
                stack.emplace_back(px + dx, py + dy);
            }
    }
}
}  // namespace

void floodFill4(Canvas& c, int x, int y, Color fill) { floodFill(c, x, y, fill, false); }
void floodFill8(Canvas& c, int x, int y, Color fill) { floodFill(c, x, y, fill, true); }

}  // namespace galaxis
