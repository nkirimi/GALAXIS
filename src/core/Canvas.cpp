#include "galaxis/Canvas.h"

#include <cmath>
#include <cstdint>
#include <fstream>

namespace galaxis {

Canvas::Canvas(int width, int height, Color background)
    : w_(width), h_(height), px_(static_cast<std::size_t>(width) * height, background) {
    resetClip();
}

void Canvas::clear(Color c) { std::fill(px_.begin(), px_.end(), c); }

void Canvas::setClip(int xmin, int ymin, int xmax, int ymax) {
    clip_ = {std::max(0, xmin), std::max(0, ymin), std::min(w_ - 1, xmax), std::min(h_ - 1, ymax)};
}

void Canvas::resetClip() { clip_ = {0, 0, w_ - 1, h_ - 1}; }

void Canvas::setPixel(int x, int y, Color c) {
    if (!visible(x, y)) return;
    px_[static_cast<std::size_t>(y) * w_ + x] = c;
}

void Canvas::blendPixel(int x, int y, Color c, double alpha) {
    if (alpha <= 0.0 || !visible(x, y)) return;
    Color& dst = px_[static_cast<std::size_t>(y) * w_ + x];
    dst = alpha >= 1.0 ? c : mix(dst, c, alpha);
}

void Canvas::addPixel(int x, int y, Color c, double k) {
    if (k <= 0.0 || !visible(x, y)) return;
    Color& dst = px_[static_cast<std::size_t>(y) * w_ + x];
    dst = {clampByte(dst.r + c.r * k), clampByte(dst.g + c.g * k), clampByte(dst.b + c.b * k)};
}

Color Canvas::getPixel(int x, int y) const {
    if (!inBounds(x, y)) return {};
    return px_[static_cast<std::size_t>(y) * w_ + x];
}

Canvas Canvas::downscaled(int factor) const {
    if (factor < 1) factor = 1;
    Canvas out(w_ / factor, h_ / factor);
    const int n = factor * factor;
    for (int y = 0; y < out.h_; ++y) {
        for (int x = 0; x < out.w_; ++x) {
            int r = 0, g = 0, b = 0;
            for (int sy = 0; sy < factor; ++sy) {
                for (int sx = 0; sx < factor; ++sx) {
                    const Color p = px_[static_cast<std::size_t>(y * factor + sy) * w_ + (x * factor + sx)];
                    r += p.r;
                    g += p.g;
                    b += p.b;
                }
            }
            out.px_[static_cast<std::size_t>(y) * out.w_ + x] = {clampByte(double(r) / n), clampByte(double(g) / n),
                                                                  clampByte(double(b) / n)};
        }
    }
    return out;
}

Canvas Canvas::resized(int nw, int nh) const {
    Canvas out(nw, nh);
    const double sx = static_cast<double>(w_) / nw, sy = static_cast<double>(h_) / nh;
    for (int y = 0; y < nh; ++y) {
        const double y0 = y * sy, y1 = (y + 1) * sy;
        for (int x = 0; x < nw; ++x) {
            const double x0 = x * sx, x1 = (x + 1) * sx;
            double r = 0, g = 0, b = 0, wsum = 0;
            for (int py = static_cast<int>(y0); py < static_cast<int>(std::ceil(y1)) && py < h_; ++py) {
                const double wy = std::min(y1, py + 1.0) - std::max(y0, static_cast<double>(py));
                for (int px = static_cast<int>(x0); px < static_cast<int>(std::ceil(x1)) && px < w_; ++px) {
                    const double wx = std::min(x1, px + 1.0) - std::max(x0, static_cast<double>(px));
                    const double wgt = wx * wy;
                    const Color p = px_[static_cast<std::size_t>(py) * w_ + px];
                    r += p.r * wgt;
                    g += p.g * wgt;
                    b += p.b * wgt;
                    wsum += wgt;
                }
            }
            out.px_[static_cast<std::size_t>(y) * nw + x] = {clampByte(r / wsum), clampByte(g / wsum), clampByte(b / wsum)};
        }
    }
    return out;
}

void Canvas::blit(const Canvas& src, int x, int y) {
    for (int sy = 0; sy < src.h_; ++sy)
        for (int sx = 0; sx < src.w_; ++sx) setPixel(x + sx, y + sy, src.px_[static_cast<std::size_t>(sy) * src.w_ + sx]);
}

// 24-bit uncompressed BMP: 14-byte file header + 40-byte info header,
// rows stored bottom-up, each row padded to a multiple of 4 bytes.
bool Canvas::saveBMP(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    const std::uint32_t rowSize = (static_cast<std::uint32_t>(w_) * 3 + 3) & ~3u;
    const std::uint32_t dataSize = rowSize * static_cast<std::uint32_t>(h_);
    const std::uint32_t fileSize = 54 + dataSize;

    auto put16 = [&](std::uint16_t v) {
        f.put(static_cast<char>(v & 0xFF));
        f.put(static_cast<char>(v >> 8));
    };
    auto put32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) f.put(static_cast<char>((v >> (8 * i)) & 0xFF));
    };

    f.put('B');
    f.put('M');
    put32(fileSize);
    put16(0);
    put16(0);
    put32(54);                                // offset to pixel data
    put32(40);                                // info header size
    put32(static_cast<std::uint32_t>(w_));
    put32(static_cast<std::uint32_t>(h_));    // positive = bottom-up
    put16(1);                                 // colour planes
    put16(24);                                // bits per pixel
    put32(0);                                 // no compression
    put32(dataSize);
    put32(2835);                              // 72 DPI
    put32(2835);
    put32(0);
    put32(0);

    std::vector<char> row(rowSize, 0);
    for (int y = h_ - 1; y >= 0; --y) {
        for (int x = 0; x < w_; ++x) {
            const Color c = px_[static_cast<std::size_t>(y) * w_ + x];
            row[x * 3 + 0] = static_cast<char>(c.b);
            row[x * 3 + 1] = static_cast<char>(c.g);
            row[x * 3 + 2] = static_cast<char>(c.r);
        }
        f.write(row.data(), rowSize);
    }
    return static_cast<bool>(f);
}

bool Canvas::savePPM(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << "P6\n" << w_ << ' ' << h_ << "\n255\n";
    for (const Color& c : px_) {
        f.put(static_cast<char>(c.r));
        f.put(static_cast<char>(c.g));
        f.put(static_cast<char>(c.b));
    }
    return static_cast<bool>(f);
}

}  // namespace galaxis
