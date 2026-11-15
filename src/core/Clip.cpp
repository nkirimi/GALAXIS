// Clip.cpp - Cohen-Sutherland line clipping.
#include "galaxis/Clip.h"

#include <cmath>

#include "galaxis/Raster.h"

namespace galaxis {

int outCode(double x, double y, const ClipRect& r) {
    int code = kInside;
    if (x < r.xmin)
        code |= kLeft;
    else if (x > r.xmax)
        code |= kRight;
    if (y < r.ymin)
        code |= kTop;
    else if (y > r.ymax)
        code |= kBottom;
    return code;
}

bool cohenSutherlandClip(double& x0, double& y0, double& x1, double& y1, const ClipRect& r) {
    int c0 = outCode(x0, y0, r);
    int c1 = outCode(x1, y1, r);
    while (true) {
        if ((c0 | c1) == 0) return true;   // trivially accept: both inside
        if ((c0 & c1) != 0) return false;  // trivially reject: share an outside zone

        // At least one endpoint is outside: move it onto the window edge.
        const int out = c0 ? c0 : c1;
        double x = 0, y = 0;
        if (out & kTop) {
            x = x0 + (x1 - x0) * (r.ymin - y0) / (y1 - y0);
            y = r.ymin;
        } else if (out & kBottom) {
            x = x0 + (x1 - x0) * (r.ymax - y0) / (y1 - y0);
            y = r.ymax;
        } else if (out & kRight) {
            y = y0 + (y1 - y0) * (r.xmax - x0) / (x1 - x0);
            x = r.xmax;
        } else {  // kLeft
            y = y0 + (y1 - y0) * (r.xmin - x0) / (x1 - x0);
            x = r.xmin;
        }
        if (out == c0) {
            x0 = x;
            y0 = y;
            c0 = outCode(x0, y0, r);
        } else {
            x1 = x;
            y1 = y;
            c1 = outCode(x1, y1, r);
        }
    }
}

void drawLineClipped(Canvas& c, double x0, double y0, double x1, double y1, const ClipRect& r, Color col,
                     double alpha) {
    if (!cohenSutherlandClip(x0, y0, x1, y1, r)) return;
    drawLine(c, static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
             static_cast<int>(std::lround(x1)), static_cast<int>(std::lround(y1)), col, alpha);
}

}  // namespace galaxis
