// App.cpp - CP3 application logic.
#include "App.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace galaxis {

namespace {
const double kSpeeds[] = {1, 2, 5, 10, 20, 50, 100, 200, 365};
}

bool App::init(std::string& error) { return renderer.init(error); }

void App::showTitle(int fbW, int fbH) {
    UiState title;
    title.splash = 1.0;
    title.blackout = 1.0;
    renderer.render(world, title, fbW, fbH);
}

void App::bake() { renderer.bakeTextures(); }

void App::frame(double dt, int winW, int winH, int fbW, int fbH) {
    winW_ = std::max(1, winW);
    winH_ = std::max(1, winH);
    fbW_ = std::max(1, fbW);
    fbH_ = std::max(1, fbH);
    world.aspect = static_cast<double>(fbW_) / fbH_;
    updateIntro(dt);
    world.update(dt);
    toastTimer_ = std::max(0.0, toastTimer_ - dt);
    ui.toastAlpha = std::min(1.0, toastTimer_ / 0.4);
    renderer.render(world, ui, fbW_, fbH_);
}

// ---------------------------------------------------------------- intro
// The title card fades in, the scene fades in behind it, and the camera
// sweeps down from high above to the default view. Any input skips it.
void App::updateIntro(double dt) {
    if (introDone_) return;
    introT_ += dt;
    const double t = introT_;
    ui.splash = smoothstep(0.0, 0.7, t) * (1.0 - smoothstep(3.0, 4.0, t));
    ui.blackout = 1.0 - smoothstep(0.5, 2.4, t);
    const double e = easeInOutCubic((t - 0.3) / 4.2);
    OrbitCamera c;
    c.distance = 300 + (c.distance - 300) * e;
    c.pitch = radians(62) + (c.pitch - radians(62)) * e;
    c.yaw = -1.3 * (1 - e);
    world.camera = c;
    if (t > 4.6) finishIntro();
}

void App::finishIntro() {
    if (introDone_) return;
    introDone_ = true;
    world.camera = OrbitCamera{};
    ui.splash = 0;
    ui.blackout = 0;
}

// ---------------------------------------------------------------- input
void App::mouseDown(Mouse b, double x, double y) {
    if (b == Mouse::Left) {
        leftDown_ = true;
        dragged_ = false;
        pressX_ = x;
        pressY_ = y;
    } else {
        rightDown_ = true;
    }
    lastX_ = x;
    lastY_ = y;
}

void App::mouseUp(Mouse b, double x, double y) {
    if (b == Mouse::Right) {
        rightDown_ = false;
        return;
    }
    leftDown_ = false;
    if (!introDone_) {
        finishIntro();
        return;
    }
    if (!dragged_) click(x, y);
}

void App::mouseMove(double x, double y) {
    const double dx = x - lastX_, dy = y - lastY_;
    lastX_ = x;
    lastY_ = y;
    if (leftDown_ && std::hypot(x - pressX_, y - pressY_) > 4) dragged_ = true;
    if (!introDone_ || world.phase() != Galaxis::Phase::Overview) return;  // camera is locked while focused
    if (leftDown_ && dragged_) world.camera.rotate(-dx * 0.006, dy * 0.006);
    if (rightDown_) world.camera.pan(dx / winH_, dy / winH_);
}

void App::scroll(double steps) {
    if (!introDone_) finishIntro();
    if (world.phase() != Galaxis::Phase::Overview) return;
    world.camera.zoom(std::pow(0.88, steps));
}

void App::click(double x, double y) {
    // Window coordinates -> framebuffer pixels (they differ on high-DPI screens).
    const double fx = x * fbW_ / winW_, fy = y * fbH_ / winH_;
    const bool panelShown = world.focus() >= 0 && world.panelAlpha() > 0.5;
    if (panelShown && renderer.hitsCloseButton(world, fx, fy, fbW_, fbH_)) {
        world.deselect();
        return;
    }
    if (panelShown && renderer.panelRect(world, fbW_, fbH_).contains(fx, fy)) return;

    // Cast a ray from the eye through the clicked pixel and find the body it hits.
    const double ndcX = fx / fbW_ * 2 - 1, ndcY = 1 - fy / fbH_ * 2;
    const int hit = world.pick(world.rayThrough(ndcX, ndcY), fbH_, 12.0 * fbH_ / 720.0);
    if (hit >= 0)
        world.select(hit);
    else if (world.focus() >= 0)
        world.deselect();
}

void App::key(Key k, int digit) {
    if (!introDone_) finishIntro();
    Galaxis& g = world;
    switch (k) {
        case Key::Escape:
            if (g.focus() >= 0)
                g.deselect();
            else
                ui.help = false;
            break;
        case Key::Space:
            g.paused = !g.paused;
            toast(g.paused ? "Paused" : "Playing");
            break;
        case Key::Faster: changeSpeed(+1); break;
        case Key::Slower: changeSpeed(-1); break;
        case Key::Orbits:
            g.showOrbits = !g.showOrbits;
            toast(g.showOrbits ? "Orbit lines on" : "Orbit lines off");
            break;
        case Key::Reset:
            if (g.phase() == Galaxis::Phase::Overview) g.camera = OrbitCamera{};
            break;
        case Key::Help: ui.help = !ui.help; break;
        case Key::Fullscreen: fullscreenRequested = true; break;
        case Key::Quit: quitRequested = true; break;
        case Key::Digit0: g.select(kSunIndex); break;
        case Key::Digit1:
            if (digit >= 1 && digit <= 8) g.select(digit - 1);
            break;
    }
}

void App::toast(const std::string& msg) {
    ui.toast = msg;
    toastTimer_ = 1.6;
}

void App::changeSpeed(int dir) {
    const int n = static_cast<int>(sizeof kSpeeds / sizeof kSpeeds[0]);
    int i = 0;
    while (i < n - 1 && kSpeeds[i] < world.daysPerSecond) ++i;
    i = std::clamp(i + dir, 0, n - 1);
    world.daysPerSecond = kSpeeds[i];
    char buf[64];
    std::snprintf(buf, sizeof buf, "%g %s / second", kSpeeds[i], kSpeeds[i] == 1 ? "day" : "days");
    toast(buf);
}

}  // namespace galaxis
