// Galaxis - CP2 Transformation and Animation
// Pure C++17 (standard library only).
//
// Renders the transformation test sheets, key stills, and an animated GIF in
// which the planets orbit and spin, the camera circles the system, and a
// scripted "user" clicks Earth and then Saturn: each time the camera flies in
// along a Bezier curve, the facts panel fades in, and the camera flies back.
//
// Usage:  galaxis_cp2 [--out DIR] [--frames] [--small]
//   --frames  also save every animation frame as a BMP
//   --small   quicker, smaller GIF (640x360)
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "galaxis/Canvas.h"
#include "galaxis/Components.h"
#include "galaxis/DemosCP2.h"
#include "galaxis/Gif.h"
#include "galaxis/Raster.h"
#include "galaxis/Renderer3D.h"
#include "galaxis/World.h"

using namespace galaxis;

namespace {

constexpr int kW = 1280, kH = 720;
constexpr int kDelayCs = 7;  // 7/100 s per frame, about 14 fps
constexpr double kDt = kDelayCs / 100.0;

// --------------------------------------------------------------- cursor
void drawCursor(Canvas& c, Point tip) {
    std::vector<Point> arrow = {{0, 0}, {0, 19}, {4.5, 14.5}, {7.5, 21}, {10.5, 19.6}, {7.6, 13.2}, {13.5, 13.2}};
    for (Point& p : arrow) p = p + tip;
    std::vector<Point> shadow = arrow;
    for (Point& p : shadow) p = p + Point{1.5, 2};
    fillPolygon(c, shadow, {0, 0, 0}, 0.45);
    fillPolygon(c, arrow, {245, 248, 255});
    drawPolygon(c, arrow, {20, 24, 40});
}

void drawRipple(Canvas& c, Point at, double t) {  // t: 0..1 after the click
    if (t < 0 || t > 1) return;
    const int r = static_cast<int>(6 + 22 * t);
    drawDashedCircle(c, static_cast<int>(at.x), static_cast<int>(at.y), r, colors::Accent, 4, 3, 1.0 - t);
    drawCircle(c, static_cast<int>(at.x), static_cast<int>(at.y), r / 2, colors::White, 0.6 * (1.0 - t));
}

Point screenOf(const Galaxis& g, int body) {
    const Projector pr(g.pose(), kFovY, kW, kH);
    Point p{kW / 2.0, kH / 2.0};
    pr.toScreen(pr.toView(g.body(body).position), p);
    return p;
}

Point closeButton() { return {kW - 60.0 - 38.0, (kH - 560) / 2.0 + 38.0}; }

// --------------------------------------------------------------- script
// A tiny timeline: each event fires once when its time is reached.
struct Script {
    struct Click {
        double time;
        int body;  // -1 = the panel's close button
    };
    std::vector<Click> clicks = {{5.0, 2}, {10.6, -1}, {13.4, 5}, {18.2, -1}};
    double end = 20.6;
    double cursorLead = 1.1;  // the cursor starts moving this long before a click
};

struct Animator {
    Galaxis g;
    Script script;
    double t = 0;
    std::size_t nextClick = 0;
    Point cursor{1040, 640};
    Point cursorFrom{1040, 640};
    double lastClickT = -10;
    Point lastClickAt{};

    Animator() {
        g.daysPerSecond = 25;
        g.camera.yaw = -0.30;
    }

    Point targetOf(const Script::Click& k) const { return k.body >= 0 ? screenOf(g, k.body) : closeButton(); }

    void step() {
        // Camera slowly circles the system while in the overview (a rotation
        // about the y axis applied to the orbit camera every frame).
        g.camera.yaw += 0.022 * kDt * 10 / 7;
        g.camera.pitch = radians(24.0 + 4.0 * std::sin(t * 0.35));

        if (nextClick < script.clicks.size()) {
            const Script::Click& k = script.clicks[nextClick];
            const double start = k.time - script.cursorLead;
            if (t < start) cursorFrom = cursor;
            if (t >= start) {
                const double u = easeInOutCubic((t - start) / (script.cursorLead - 0.15));
                const Point target = targetOf(k);
                cursor = cursorFrom + (target - cursorFrom) * u;
            }
            if (t >= k.time) {
                lastClickT = t;
                lastClickAt = cursor;
                if (k.body >= 0)
                    g.select(k.body);
                else
                    g.deselect();
                ++nextClick;
                cursorFrom = cursor;
            }
        }
        g.update(kDt);
        t += kDt;
    }

    bool cursorVisible() const {
        // Hidden while flying; visible while it travels to the next click and shortly after.
        if (t - lastClickT < 0.6) return true;
        if (nextClick < script.clicks.size() && t >= script.clicks[nextClick].time - script.cursorLead) return true;
        return false;
    }

    Canvas frame() const {
        Canvas c(kW, kH);
        renderWorld(c, g);
        drawRipple(c, lastClickAt, (t - lastClickT) / 0.5);
        if (cursorVisible()) drawCursor(c, cursor);
        return c;
    }
};

}  // namespace

int main(int argc, char** argv) {
    std::string outDir = "output";
    bool saveFrames = false, small = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--out" && i + 1 < argc)
            outDir = argv[++i];
        else if (a == "--frames")
            saveFrames = true;
        else if (a == "--small")
            small = true;
        else {
            std::cerr << "usage: " << argv[0] << " [--out DIR] [--frames] [--small]\n";
            return 1;
        }
    }
    const std::string dir = outDir + "/cp2";
    std::error_code ec;
    std::filesystem::create_directories(dir + (saveFrames ? "/frames" : ""), ec);

    int failures = 0;
    auto save = [&](const std::string& name, const Canvas& c) {
        const bool ok = c.saveBMP(dir + "/" + name + ".bmp");
        std::cout << (ok ? "  wrote " : "  FAILED ") << dir << "/" << name << ".bmp\n";
        if (!ok) ++failures;
    };

    std::cout << "Galaxis CP2 - rendering...\n";
    save("07_transform2d", demoTransform2D());
    save("08_transform3d", demoTransform3D());

    // Key stills of the 3-D scene.
    {
        Galaxis g;
        Canvas c(kW, kH);
        renderWorld(c, g);
        save("overview_3d", c);
        const int bodies[] = {2, 5, 8};
        const char* names[] = {"focus_earth", "focus_saturn", "focus_sun"};
        for (int i = 0; i < 3; ++i) {
            Galaxis f;
            f.select(bodies[i]);
            for (int k = 0; k < 40; ++k) f.update(0.05);
            Canvas fc(kW, kH);
            renderWorld(fc, f);
            save(names[i], fc);
        }
        Galaxis m;
        m.select(2);
        for (int k = 0; k < 17; ++k) m.update(0.05);
        Canvas mc(kW, kH);
        renderWorld(mc, m);
        save("flyin_midway", mc);
    }

    // Animation. Pass 1 renders a few sample frames to build the palette.
    const int gw = small ? 640 : 960, gh = small ? 360 : 540;
    std::cout << "  building palette...\n";
    std::vector<Canvas> samples;
    {
        Animator a;
        int n = 0;
        while (a.t < a.script.end) {
            if (n % 24 == 0) samples.push_back(a.frame().resized(gw, gh));
            a.step();
            ++n;
        }
    }
    std::vector<const Canvas*> sp;
    for (const Canvas& s : samples) sp.push_back(&s);
    const std::string gifPath = dir + "/galaxis_cp2.gif";
    GifWriter gif(gifPath, gw, gh, medianCutPalette(sp, 255));

    std::cout << "  rendering animation";
    Animator a;
    int n = 0;
    while (a.t < a.script.end) {
        const Canvas full = a.frame();
        if (saveFrames) {
            char name[64];
            std::snprintf(name, sizeof name, "/frames/frame_%04d.bmp", n);
            full.saveBMP(dir + name);
        }
        gif.addFrame(full.resized(gw, gh), kDelayCs);
        a.step();
        if (++n % 20 == 0) std::cout << '.' << std::flush;
    }
    gif.close();
    std::cout << "\n  wrote " << gifPath << " (" << n << " frames)\n";

    std::cout << (failures ? "Done with errors.\n" : "Done.\n");
    return failures ? 1 : 0;
}
