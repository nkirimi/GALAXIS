// Gif.cpp - GIF89a encoder: median-cut palette, ordered dither, LZW.
#include "galaxis/Gif.h"

#include <algorithm>
#include <array>

namespace galaxis {

// ================================================================ palette
std::vector<Color> medianCutPalette(const std::vector<const Canvas*>& samples, int maxColors) {
    std::vector<Color> pixels;
    for (const Canvas* c : samples)
        for (int y = 0; y < c->height(); y += 2)
            for (int x = (y / 2) % 2; x < c->width(); x += 2) pixels.push_back(c->getPixel(x, y));

    struct Box {
        std::size_t begin, end;
    };
    std::vector<Box> boxes{{0, pixels.size()}};
    auto channel = [](Color c, int ch) { return ch == 0 ? c.r : (ch == 1 ? c.g : c.b); };

    // Repeatedly split the box with the widest colour range at its median.
    while (static_cast<int>(boxes.size()) < maxColors) {
        int bestBox = -1, bestCh = 0, bestRange = 0;
        for (int i = 0; i < static_cast<int>(boxes.size()); ++i) {
            const Box& b = boxes[static_cast<std::size_t>(i)];
            if (b.end - b.begin < 2) continue;
            for (int ch = 0; ch < 3; ++ch) {
                int lo = 255, hi = 0;
                for (std::size_t k = b.begin; k < b.end; ++k) {
                    const int v = channel(pixels[k], ch);
                    lo = std::min(lo, v);
                    hi = std::max(hi, v);
                }
                // Weight by population so busy regions get more colours.
                const int score = (hi - lo) * static_cast<int>(std::min<std::size_t>(b.end - b.begin, 1 << 16) >> 6);
                if (hi - lo > 0 && score >= bestRange) {
                    bestRange = score;
                    bestBox = i;
                    bestCh = ch;
                }
            }
        }
        if (bestBox < 0) break;
        Box b = boxes[static_cast<std::size_t>(bestBox)];
        const std::size_t mid = (b.begin + b.end) / 2;
        std::nth_element(pixels.begin() + static_cast<std::ptrdiff_t>(b.begin), pixels.begin() + static_cast<std::ptrdiff_t>(mid),
                         pixels.begin() + static_cast<std::ptrdiff_t>(b.end),
                         [&](Color a, Color c) { return channel(a, bestCh) < channel(c, bestCh); });
        boxes[static_cast<std::size_t>(bestBox)] = {b.begin, mid};
        boxes.push_back({mid, b.end});
    }

    std::vector<Color> palette;
    for (const Box& b : boxes) {
        if (b.end == b.begin) continue;
        long r = 0, g = 0, bl = 0;
        for (std::size_t k = b.begin; k < b.end; ++k) {
            r += pixels[k].r;
            g += pixels[k].g;
            bl += pixels[k].b;
        }
        const long n = static_cast<long>(b.end - b.begin);
        palette.push_back({static_cast<std::uint8_t>(r / n), static_cast<std::uint8_t>(g / n), static_cast<std::uint8_t>(bl / n)});
    }
    return palette;
}

// ================================================================ writer
namespace {

void put16(std::ofstream& f, int v) {
    f.put(static_cast<char>(v & 0xFF));
    f.put(static_cast<char>((v >> 8) & 0xFF));
}

constexpr std::uint8_t kTransparent = 255;

const int kBayer4[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

}  // namespace

GifWriter::GifWriter(const std::string& path, int width, int height, std::vector<Color> palette)
    : out_(path, std::ios::binary), w_(width), h_(height), palette_(std::move(palette)) {
    palette_.resize(256, Color{0, 0, 0});

    // Nearest palette entry for every cell of a 32x32x32 colour cube.
    lut_.resize(32 * 32 * 32);
    for (int r = 0; r < 32; ++r)
        for (int g = 0; g < 32; ++g)
            for (int b = 0; b < 32; ++b) {
                const int cr = r * 8 + 4, cg = g * 8 + 4, cb = b * 8 + 4;
                int best = 0, bestD = 1 << 30;
                for (int i = 0; i < 255; ++i) {
                    const int dr = cr - palette_[static_cast<std::size_t>(i)].r;
                    const int dg = cg - palette_[static_cast<std::size_t>(i)].g;
                    const int db = cb - palette_[static_cast<std::size_t>(i)].b;
                    const int d = 3 * dr * dr + 4 * dg * dg + 2 * db * db;  // rough perceptual weights
                    if (d < bestD) {
                        bestD = d;
                        best = i;
                    }
                }
                lut_[static_cast<std::size_t>((r << 10) | (g << 5) | b)] = static_cast<std::uint8_t>(best);
            }

    out_.write("GIF89a", 6);
    put16(out_, w_);
    put16(out_, h_);
    out_.put(static_cast<char>(0xF7));  // global colour table, 8 bits per channel, 256 entries
    out_.put(0);                        // background colour index
    out_.put(0);                        // pixel aspect ratio
    for (const Color& c : palette_) {
        out_.put(static_cast<char>(c.r));
        out_.put(static_cast<char>(c.g));
        out_.put(static_cast<char>(c.b));
    }
    // NETSCAPE2.0 application extension: loop forever.
    const unsigned char loop[] = {0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 0x03, 0x01, 0x00, 0x00, 0x00};
    out_.write(reinterpret_cast<const char*>(loop), sizeof loop);
}

GifWriter::~GifWriter() { close(); }

std::uint8_t GifWriter::nearest(Color c) const {
    return lut_[static_cast<std::size_t>(((c.r >> 3) << 10) | ((c.g >> 3) << 5) | (c.b >> 3))];
}

void GifWriter::addFrame(const Canvas& frame, int delayCs) {
    std::vector<std::uint8_t> idx(static_cast<std::size_t>(w_) * h_);
    for (int y = 0; y < h_; ++y)
        for (int x = 0; x < w_; ++x) {
            const int d = kBayer4[y & 3][x & 3] - 8;  // -8..7
            const Color c = frame.getPixel(x, y);
            auto ch = [&](int v) { return static_cast<std::uint8_t>(std::clamp(v + d / 2, 0, 255)); };
            idx[static_cast<std::size_t>(y) * w_ + x] = nearest({ch(c.r), ch(c.g), ch(c.b)});
        }

    // Only the rectangle that changed since the previous frame is stored, and
    // unchanged pixels inside it become transparent.
    int x0 = 0, y0 = 0, x1 = w_ - 1, y1 = h_ - 1;
    if (!first_) {
        x0 = w_;
        y0 = h_;
        x1 = -1;
        y1 = -1;
        for (int y = 0; y < h_; ++y)
            for (int x = 0; x < w_; ++x)
                if (idx[static_cast<std::size_t>(y) * w_ + x] != prev_[static_cast<std::size_t>(y) * w_ + x]) {
                    x0 = std::min(x0, x);
                    x1 = std::max(x1, x);
                    y0 = std::min(y0, y);
                    y1 = std::max(y1, y);
                }
        if (x1 < 0) x0 = y0 = x1 = y1 = 0;  // identical frame: store a single transparent pixel
    }
    std::vector<std::uint8_t> sub;
    sub.reserve(static_cast<std::size_t>(x1 - x0 + 1) * (y1 - y0 + 1));
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            const std::size_t k = static_cast<std::size_t>(y) * w_ + x;
            sub.push_back(!first_ && idx[k] == prev_[k] ? kTransparent : idx[k]);
        }

    // Graphic control extension: delay, "do not dispose", transparent index.
    out_.put(0x21);
    out_.put(static_cast<char>(0xF9));
    out_.put(4);
    out_.put(static_cast<char>((1 << 2) | 1));
    put16(out_, delayCs);
    out_.put(static_cast<char>(kTransparent));
    out_.put(0);
    // Image descriptor
    out_.put(0x2C);
    put16(out_, x0);
    put16(out_, y0);
    put16(out_, x1 - x0 + 1);
    put16(out_, y1 - y0 + 1);
    out_.put(0);
    writeLzw(sub);

    prev_ = std::move(idx);
    first_ = false;
}

void GifWriter::writeLzw(const std::vector<std::uint8_t>& data) {
    const int minCode = 8, clearCode = 256, eoi = 257;
    out_.put(static_cast<char>(minCode));

    std::vector<std::uint8_t> block;
    std::uint32_t bitBuf = 0;
    int bitCount = 0;
    auto flushBlock = [&] {
        if (block.empty()) return;
        out_.put(static_cast<char>(block.size()));
        out_.write(reinterpret_cast<const char*>(block.data()), static_cast<std::streamsize>(block.size()));
        block.clear();
    };
    int codeSize = minCode + 1;
    auto emit = [&](int code) {
        bitBuf |= static_cast<std::uint32_t>(code) << bitCount;
        bitCount += codeSize;
        while (bitCount >= 8) {
            block.push_back(static_cast<std::uint8_t>(bitBuf & 0xFF));
            bitBuf >>= 8;
            bitCount -= 8;
            if (block.size() == 255) flushBlock();
        }
    };

    // Dictionary: open-addressing hash of (prefix code, next byte) -> code.
    constexpr int kHashSize = 8192;
    std::array<std::int32_t, kHashSize> keys{};
    std::array<std::int16_t, kHashSize> vals{};
    auto reset = [&] { keys.fill(-1); };
    reset();
    int nextCode = eoi + 1;

    emit(clearCode);
    if (data.empty()) {
        emit(eoi);
    } else {
        int prefix = data[0];
        for (std::size_t i = 1; i < data.size(); ++i) {
            const int c = data[i];
            const std::int32_t key = (prefix << 8) | c;
            std::uint32_t h = (static_cast<std::uint32_t>(key) * 2654435761u) >> 19;  // 13-bit hash
            bool found = false;
            while (keys[h] != -1) {
                if (keys[h] == key) {
                    prefix = vals[h];
                    found = true;
                    break;
                }
                h = (h + 1) & (kHashSize - 1);
            }
            if (found) continue;
            emit(prefix);
            if (nextCode < 4096) {
                keys[h] = key;
                vals[h] = static_cast<std::int16_t>(nextCode);
                if (nextCode == (1 << codeSize) && codeSize < 12) ++codeSize;
                ++nextCode;
            } else {
                emit(clearCode);
                reset();
                nextCode = eoi + 1;
                codeSize = minCode + 1;
            }
            prefix = c;
        }
        emit(prefix);
        emit(eoi);
    }
    if (bitCount > 0) {
        block.push_back(static_cast<std::uint8_t>(bitBuf & 0xFF));
        if (block.size() == 255) flushBlock();
    }
    flushBlock();
    out_.put(0);  // block terminator
}

bool GifWriter::close() {
    if (closed_) return ok();
    closed_ = true;
    out_.put(0x3B);  // trailer
    out_.close();
    return true;
}

}  // namespace galaxis
