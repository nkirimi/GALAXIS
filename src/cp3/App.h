// App.h - CP3 application logic: input handling, intro, toasts.
//
// Kept independent of GLFW so the same code can be driven by main.cpp (a real
// window) or by an automated test that feeds it scripted input.
#pragma once

#include <string>

#include "Renderer.h"
#include "galaxis/World.h"

namespace galaxis {

enum class Key { Escape, Space, Faster, Slower, Orbits, Reset, Help, Fullscreen, Quit, Digit0, Digit1 };
enum class Mouse { Left, Right };

class App {
public:
    bool init(std::string& error);  // shaders and meshes
    void showTitle(int fbW, int fbH);  // draw the title card once (before baking)
    void bake();                      // procedural textures

    // Called every frame with the window size (input coordinates) and the
    // framebuffer size (rendering pixels; larger on high-DPI screens).
    void frame(double dt, int winW, int winH, int fbW, int fbH);

    void mouseDown(Mouse b, double x, double y);
    void mouseUp(Mouse b, double x, double y);
    void mouseMove(double x, double y);
    void scroll(double steps);
    void key(Key k, int digit = 0);  // digit for Key::Digit1 (1..8)

    bool quitRequested = false;
    bool fullscreenRequested = false;  // main.cpp toggles and clears it

    Galaxis world;
    UiState ui;
    GLRenderer renderer;

private:
    void click(double x, double y);
    void finishIntro();
    void updateIntro(double dt);
    void toast(const std::string& msg);
    void changeSpeed(int dir);

    double introT_ = 0;
    bool introDone_ = false;
    bool leftDown_ = false, rightDown_ = false, dragged_ = false;
    double pressX_ = 0, pressY_ = 0, lastX_ = 0, lastY_ = 0;
    double toastTimer_ = 0;
    int winW_ = 1280, winH_ = 720, fbW_ = 1280, fbH_ = 720;
};

}  // namespace galaxis
