// Canvas.h - an in-memory framebuffer. Every algorithm in this project draws
// by calling setPixel / blendPixel / addPixel; nothing else touches pixels.
// The image is saved as a BMP (opens on Windows/macOS/Linux) or a PPM.
#pragma once

#include <string>
#include <vector>

#include "galaxis/Color.h"

namespace galaxis {

class Canvas {
public:
    struct Rect {
        int xmin, ymin, xmax, ymax;  // inclusive
    };

    Canvas(int width, int height, Color background = colors::Space);

    int width() const { return w_; }
    int height() const { return h_; }

    void clear(Color c);

    // Pixel access. Writes outside the canvas or outside the clip rectangle
    // are silently ignored, so callers never need bounds checks.
    void setPixel(int x, int y, Color c);
    void blendPixel(int x, int y, Color c, double alpha);  // alpha in [0,1]
    void addPixel(int x, int y, Color c, double k);        // additive light (glows)
    Color getPixel(int x, int y) const;

    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }
    bool visible(int x, int y) const {
        return x >= clip_.xmin && x <= clip_.xmax && y >= clip_.ymin && y <= clip_.ymax;
    }

    // Pixel-level clip window (a "scissor"): restricts drawing to a viewport.
    void setClip(int xmin, int ymin, int xmax, int ymax);
    void resetClip();
    Rect clip() const { return clip_; }

    // Box-filter downscale by an integer factor (for contact sheets).
    Canvas downscaled(int factor) const;
    // Area-averaging resize to any smaller size (for the CP2 animation).
    Canvas resized(int width, int height) const;
    // Copy another canvas onto this one with its top-left at (x, y).
    void blit(const Canvas& src, int x, int y);

    bool saveBMP(const std::string& path) const;
    bool savePPM(const std::string& path) const;

private:
    int w_, h_;
    std::vector<Color> px_;
    Rect clip_;
};

}  // namespace galaxis
