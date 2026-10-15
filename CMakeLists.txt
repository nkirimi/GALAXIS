cmake_minimum_required(VERSION 3.13)
project(galaxis C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB GALAXIS_CORE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
add_library(galaxis_core STATIC ${GALAXIS_CORE})
target_include_directories(galaxis_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB CP1_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/cp1/*.cpp)
add_executable(galaxis_cp1 ${CP1_SRC})
target_link_libraries(galaxis_cp1 PRIVATE galaxis_core)

file(GLOB CP2_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/cp2/*.cpp)
add_executable(galaxis_cp2 ${CP2_SRC})
target_link_libraries(galaxis_cp2 PRIVATE galaxis_core)

# ---------------------------------------------------------------- CP3
option(GALAXIS_BUILD_CP3 "Build the OpenGL prototype (needs GLFW)" ON)
if(GALAXIS_BUILD_CP3)
  find_package(glfw3 3.3 QUIET)
  if(NOT glfw3_FOUND)
    # Build GLFW from the copy in third_party.
    set(G ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/src)
    set(GLFW_SRC ${G}/context.c ${G}/init.c ${G}/input.c ${G}/monitor.c ${G}/platform.c ${G}/vulkan.c ${G}/window.c
                 ${G}/egl_context.c ${G}/osmesa_context.c ${G}/null_init.c ${G}/null_monitor.c ${G}/null_window.c
                 ${G}/null_joystick.c)
    if(WIN32)
      list(APPEND GLFW_SRC ${G}/win32_init.c ${G}/win32_joystick.c ${G}/win32_monitor.c ${G}/win32_time.c
                           ${G}/win32_thread.c ${G}/win32_window.c ${G}/win32_module.c ${G}/wgl_context.c)
      set(GLFW_DEF _GLFW_WIN32)
    elseif(APPLE)
      message(FATAL_ERROR "On macOS please install GLFW first: brew install glfw")
    else()
      list(APPEND GLFW_SRC ${G}/posix_module.c ${G}/posix_time.c ${G}/posix_thread.c ${G}/posix_poll.c
                           ${G}/linux_joystick.c ${G}/xkb_unicode.c ${G}/x11_init.c ${G}/x11_monitor.c
                           ${G}/x11_window.c ${G}/glx_context.c)
      set(GLFW_DEF _GLFW_X11)
    endif()
    add_library(glfw STATIC ${GLFW_SRC})
    target_compile_definitions(glfw PRIVATE ${GLFW_DEF})
    target_include_directories(glfw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include)
    if(WIN32)
      target_link_libraries(glfw PUBLIC gdi32)
    else()
      target_link_libraries(glfw PUBLIC ${CMAKE_DL_LIBS} pthread m)
    endif()
  endif()

  file(GLOB CP3_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/cp3/*.cpp)
  add_executable(galaxis_cp3 ${CP3_SRC})
  target_include_directories(galaxis_cp3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include)
  target_link_libraries(galaxis_cp3 PRIVATE galaxis_core glfw)
  if(APPLE)
    target_link_libraries(galaxis_cp3 PRIVATE "-framework OpenGL")
  endif()
endif()
