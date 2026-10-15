# Galaxis - Linux / macOS build.  (Windows: use build.bat)
#
#   make cp1 | cp2 | cp3 | all      build
#   make run1 | run2 | run3         build and run
#
# CP3 needs GLFW. If pkg-config finds it (apt install libglfw3-dev /
# brew install glfw) that copy is used; otherwise GLFW is compiled from
# third_party/ for X11, which needs the X11 development headers.

CXX      ?= g++
CC       ?= cc
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS += -Iinclude
UNAME    := $(shell uname -s)

CORE := $(wildcard src/core/*.cpp)
HDRS := $(wildcard include/galaxis/*.h)

all: cp1 cp2 cp3

cp1: galaxis_cp1
cp2: galaxis_cp2
cp3: galaxis_cp3

galaxis_cp1: $(CORE) $(wildcard src/cp1/*.cpp) $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(CORE) $(wildcard src/cp1/*.cpp) -o $@

galaxis_cp2: $(CORE) $(wildcard src/cp2/*.cpp) $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(CORE) $(wildcard src/cp2/*.cpp) -o $@

# ------------------------------------------------------------------ CP3
GLFW_PKG := $(shell pkg-config --exists glfw3 2>/dev/null && echo yes)
GL_INC   := -Ithird_party/glad/include
CP3_SRC  := $(CORE) $(wildcard src/cp3/*.cpp)

ifeq ($(GLFW_PKG),yes)
  GLFW_CFLAGS := $(shell pkg-config --cflags glfw3)
  GLFW_LIBS   := $(shell pkg-config --libs glfw3)
  GLFW_OBJS   :=
else
  GLFW_CFLAGS := -Ithird_party/glfw/include
  G := third_party/glfw/src
  GLFW_COMMON := $(G)/context.c $(G)/init.c $(G)/input.c $(G)/monitor.c $(G)/platform.c $(G)/vulkan.c $(G)/window.c \
                 $(G)/egl_context.c $(G)/osmesa_context.c $(G)/null_init.c $(G)/null_monitor.c $(G)/null_window.c \
                 $(G)/null_joystick.c
  ifeq ($(UNAME),Darwin)
    $(error On macOS please install GLFW first:  brew install glfw)
  endif
  GLFW_C    := $(GLFW_COMMON) $(G)/posix_module.c $(G)/posix_time.c $(G)/posix_thread.c $(G)/posix_poll.c \
               $(G)/linux_joystick.c $(G)/xkb_unicode.c $(G)/x11_init.c $(G)/x11_monitor.c $(G)/x11_window.c $(G)/glx_context.c
  GLFW_OBJS := $(patsubst $(G)/%.c,build/glfw/%.o,$(GLFW_C))
  GLFW_LIBS := -ldl -lpthread -lm
endif

ifeq ($(UNAME),Darwin)
  GL_LIBS := -framework OpenGL
else
  GL_LIBS :=
endif

build/glfw/%.o: third_party/glfw/src/%.c
	@mkdir -p build/glfw
	$(CC) -c -O2 -D_GLFW_X11 -Ithird_party/glfw/include $< -o $@

galaxis_cp3: $(CP3_SRC) $(wildcard src/cp3/*.h) $(HDRS) $(GLFW_OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(GL_INC) $(GLFW_CFLAGS) $(CP3_SRC) $(GLFW_OBJS) $(GLFW_LIBS) $(GL_LIBS) -o $@

run1: cp1
	./galaxis_cp1
run2: cp2
	./galaxis_cp2
run3: cp3
	./galaxis_cp3

clean:
	rm -rf build galaxis_cp1 galaxis_cp2 galaxis_cp3

.PHONY: all cp1 cp2 cp3 run1 run2 run3 clean
