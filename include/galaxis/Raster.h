// Raster.h - the core scan-conversion algorithms, written from scratch.
//
// Each algorithm is implemented once as a "generator" that reports the pixels
// it produces through a callback. The Canvas helpers below are thin wrappers.
// This keeps the maths in one place and lets components reuse it for effects
// (dashed lines, fading trails, shaded spans) without copying the algorithm.
#pragma once

#include <functional>
#include <vector>

#include "galaxis/Canvas.h"
#include "galaxis/Math.h"

namespace galaxis {

// Callback types
using StepPlotFn = std::function<void(int x, int y, int step)>;                // lines
using CirclePlotFn = std::function<void(int x, int y, int step, int octant)>;  // circles
using PlotFn = std::function<void(int x, int y)>;                              // ellipses
using SpanFn = std::function<void(int y, int xLeft, int xRight)>;              // fills (inclusive)

// ---------------------------------------------------------------- Lines
// Digital Differential Analyzer: steps along the major axis in unit
// increments and rounds the floating-point minor coordinate.
void ddaLine(int x0, int y0, int x1, int y1, const StepPlotFn& plot);

// Bresenham: integer-only, uses a decision parameter p = 2*dMinor - dMajor.
// Handles all eight octants.
void bresenhamLine(int x0, int y0, int x1, int y1, const StepPlotFn& plot);

void drawLineDDA(Canvas& c, int x0, int y0, int x1, int y1, Color col, double alpha = 1.0);
void drawLine(Canvas& c, int x0, int y0, int x1, int y1, Color col, double alpha = 1.0);  // Bresenham
void drawThickLine(Canvas& c, int x0, int y0, int x1, int y1, int thickness, Color col, double alpha = 1.0);

// -------------------------------------------------------------- Circles
// Midpoint circle: computes one octant with decision parameter p = 1 - r
// and mirrors each point into all eight octants.
void midpointCircle(int cx, int cy, int r, const CirclePlotFn& plot);

// Filled circle: runs the midpoint algorithm once to find the half-width of
// every scanline, then emits exactly one horizontal span per row.
void circleSpans(int cx, int cy, int r, const SpanFn& span);

void drawCircle(Canvas& c, int cx, int cy, int r, Color col, double alpha = 1.0);
void fillCircle(Canvas& c, int cx, int cy, int r, Color col, double alpha = 1.0);
void drawDashedCircle(Canvas& c, int cx, int cy, int r, Color col, int dash, int gap, double alpha = 1.0);

// ------------------------------------------------------------- Ellipses
// Which half of an ellipse to draw. In Galaxis the orbital plane is viewed
// from slightly above, so the Top half of an orbit is the far side (behind
// the Sun) and the Bottom half is the near side (in front of the Sun).
enum class Half { Both, Top, Bottom };

// Midpoint ellipse: region 1 (slope > -1) steps in x, region 2 steps in y.
// Mirrors each point into four quadrants.
void midpointEllipse(int cx, int cy, int rx, int ry, const PlotFn& plot, Half half = Half::Both);
void ellipseSpans(int cx, int cy, int rx, int ry, const SpanFn& span, Half half = Half::Both);

void drawEllipse(Canvas& c, int cx, int cy, int rx, int ry, Color col, double alpha = 1.0, Half half = Half::Both);
void fillEllipse(Canvas& c, int cx, int cy, int rx, int ry, Color col, double alpha = 1.0, Half half = Half::Both);

// ------------------------------------------------------- Polygon filling
// Scanline polygon fill with an Edge Table (edges bucketed by starting
// scanline) and an Active Edge Table (edges crossing the current scanline,
// x advanced incrementally by 1/slope). Even-odd rule, pixel-centre sampling.
void scanlinePolygon(const std::vector<Point>& poly, const SpanFn& span);

void fillPolygon(Canvas& c, const std::vector<Point>& poly, Color col, double alpha = 1.0);
void drawPolygon(Canvas& c, const std::vector<Point>& poly, Color col, double alpha = 1.0);  // closed outline

// Seed fills (iterative with an explicit stack, so large regions cannot
// overflow the call stack the way the recursive textbook version can).
void boundaryFill4(Canvas& c, int x, int y, Color fill, Color boundary);  // stops at boundary colour
// Flood fill replaces the seed pixel's colour throughout its connected region.
// Note: an 8-connected fill leaks through the diagonal steps of a 1-pixel
// Bresenham outline, so regions bounded by thin lines need the 4-connected one.
void floodFill4(Canvas& c, int x, int y, Color fill);
void floodFill8(Canvas& c, int x, int y, Color fill);

}  // namespace galaxis
