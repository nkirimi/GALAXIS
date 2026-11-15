// Galaxis - CP3 OpenGL prototype
// GLFW window + OpenGL 3.3 core profile.
//
// The simulation (orbits, spin, camera, Bezier fly-to, ray picking) is the
// same pure C++ code as CP2; the procedural textures and the facts panel are
// drawn with the CP1 code. This file only opens the window and forwards
// GLFW events to App.
//
// Controls: drag = orbit, right-drag = pan, scroll = zoom, click a body = fly
// in and show its facts, Esc = back, Space = pause, +/- = speed, O = orbit
// lines, R = reset camera, F = fullscreen, H = help, 1-8 / 0 = jump, Q = quit.
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "App.h"

using namespace galaxis;

namespace {

App* appOf(GLFWwindow* w) { return static_cast<App*>(glfwGetWindowUserPointer(w)); }

void onMouseButton(GLFWwindow* w, int button, int action, int /*mods*/) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    const Mouse b = (button == GLFW_MOUSE_BUTTON_LEFT) ? Mouse::Left : Mouse::Right;
    if (action == GLFW_PRESS)
        appOf(w)->mouseDown(b, x, y);
    else if (action == GLFW_RELEASE)
        appOf(w)->mouseUp(b, x, y);
}

void onCursor(GLFWwindow* w, double x, double y) { appOf(w)->mouseMove(x, y); }

void onScroll(GLFWwindow* w, double /*xoff*/, double yoff) { appOf(w)->scroll(yoff); }

void onKey(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    App& a = *appOf(w);
    switch (key) {
        case GLFW_KEY_ESCAPE: a.key(Key::Escape); break;
        case GLFW_KEY_SPACE: a.key(Key::Space); break;
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD: a.key(Key::Faster); break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT: a.key(Key::Slower); break;
        case GLFW_KEY_O: a.key(Key::Orbits); break;
        case GLFW_KEY_R: a.key(Key::Reset); break;
        case GLFW_KEY_H: a.key(Key::Help); break;
        case GLFW_KEY_F:
        case GLFW_KEY_F11: a.key(Key::Fullscreen); break;
        case GLFW_KEY_Q: a.key(Key::Quit); break;
        case GLFW_KEY_0:
        case GLFW_KEY_KP_0: a.key(Key::Digit0); break;
        default:
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) a.key(Key::Digit1, key - GLFW_KEY_0);
            if (key >= GLFW_KEY_KP_1 && key <= GLFW_KEY_KP_8) a.key(Key::Digit1, key - GLFW_KEY_KP_0);
            break;
    }
}

void toggleFullscreen(GLFWwindow* window, int saved[4]) {
    if (glfwGetWindowMonitor(window)) {
        glfwSetWindowMonitor(window, nullptr, saved[0], saved[1], saved[2], saved[3], 0);
    } else {
        glfwGetWindowPos(window, &saved[0], &saved[1]);
        glfwGetWindowSize(window, &saved[2], &saved[3]);
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primary);
        glfwSetWindowMonitor(window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

}  // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Could not initialise GLFW.\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // required on macOS
    glfwWindowHint(GLFW_SAMPLES, 4);                        // multisample anti-aliasing

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Galaxis - Group 4", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Could not open an OpenGL 3.3 window. Is your graphics driver up to date?\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Could not load OpenGL functions.\n");
        return 1;
    }
    glEnable(GL_MULTISAMPLE);

    App app;
    std::string error;
    if (!app.init(error)) {
        std::fprintf(stderr, "Shader error:\n%s\n", error.c_str());
        return 1;
    }
    glfwSetWindowUserPointer(window, &app);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onCursor);
    glfwSetScrollCallback(window, onScroll);
    glfwSetKeyCallback(window, onKey);

    // Show the title card at once, then bake the textures behind it.
    int fw, fh, ww, wh;
    glfwGetFramebufferSize(window, &fw, &fh);
    app.showTitle(fw, fh);
    glfwSwapBuffers(window);
    glfwPollEvents();
    app.bake();

    int saved[4] = {100, 100, 1280, 720};
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window) && !app.quitRequested) {
        const double now = glfwGetTime();
        const double dt = std::min(0.1, now - last);
        last = now;
        if (app.fullscreenRequested) {
            app.fullscreenRequested = false;
            toggleFullscreen(window, saved);
        }
        glfwGetFramebufferSize(window, &fw, &fh);
        glfwGetWindowSize(window, &ww, &wh);
        if (fw > 0 && fh > 0) app.frame(dt, ww, wh, fw, fh);  // skipped while minimised
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
