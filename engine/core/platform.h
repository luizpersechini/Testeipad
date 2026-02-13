/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Platform Abstraction Layer
 *
 * Based on the original C&C Tiberian Dawn source code by Westwood Studios
 * Original code released under GPL v3 by Electronic Arts
 *
 * This abstraction layer enables the game to run on:
 *   - Desktop (Windows/macOS/Linux) via SDL2 + OpenGL
 *   - Web browsers via Emscripten + WebGL
 */

#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <memory>
#include <vector>

// Platform detection
#if defined(__EMSCRIPTEN__)
    #define PLATFORM_WEB 1
    #define PLATFORM_NAME "Web"
#elif defined(_WIN32)
    #define PLATFORM_DESKTOP 1
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define PLATFORM_DESKTOP 1
    #define PLATFORM_MACOS 1
    #define PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define PLATFORM_DESKTOP 1
    #define PLATFORM_LINUX 1
    #define PLATFORM_NAME "Linux"
#endif

namespace CnC {

// Forward declarations
class Renderer;
class AudioSystem;
class InputSystem;

// ============================================================
// Core Types
// ============================================================

struct WindowConfig {
    std::string title = "Command & Conquer: Tiberian Dawn";
    int width = 1280;
    int height = 720;
    int logical_width = 640;   // Original game resolution
    int logical_height = 400;  // Original game resolution
    bool fullscreen = false;
    bool vsync = true;
    int scale_factor = 2;
};

struct Color {
    uint8_t r, g, b, a;
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    static Color Black()   { return {0, 0, 0, 255}; }
    static Color White()   { return {255, 255, 255, 255}; }
    static Color Red()     { return {255, 0, 0, 255}; }
    static Color Green()   { return {0, 255, 0, 255}; }
    static Color Blue()    { return {0, 0, 255, 255}; }
    static Color Yellow()  { return {255, 255, 0, 255}; }

    // GDI faction color (gold)
    static Color GDI()     { return {218, 165, 32, 255}; }
    // Nod faction color (red)
    static Color Nod()     { return {180, 0, 0, 255}; }
    // Tiberium green
    static Color Tiberium(){ return {0, 200, 0, 255}; }
};

struct Rect {
    int x, y, w, h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    bool intersects(const Rect& other) const {
        return x < other.x + other.w && x + w > other.x &&
               y < other.y + other.h && y + h > other.y;
    }
};

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}
};

// ============================================================
// Input Types
// ============================================================

enum class MouseButton {
    Left = 0,
    Middle = 1,
    Right = 2
};

enum class KeyCode {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5,
    Num6, Num7, Num8, Num9,
    Escape, Enter, Space, Tab, Backspace,
    Left, Right, Up, Down,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
    Delete, Home, End, PageUp, PageDown
};

struct InputEvent {
    enum class Type {
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel,
        KeyDown,
        KeyUp,
        Quit,
        WindowResize
    };

    Type type;
    int mouse_x, mouse_y;
    int wheel_delta;
    MouseButton button;
    KeyCode key;
    bool shift, ctrl, alt;
};

// ============================================================
// Texture Handle
// ============================================================

using TextureID = uint32_t;
constexpr TextureID INVALID_TEXTURE = 0;

struct TextureInfo {
    TextureID id = INVALID_TEXTURE;
    int width = 0;
    int height = 0;
};

// ============================================================
// Platform Interface
// ============================================================

class Platform {
public:
    virtual ~Platform() = default;

    // Lifecycle
    virtual bool init(const WindowConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool should_quit() const = 0;

    // Event handling
    virtual void poll_events() = 0;
    virtual bool pop_event(InputEvent& event) = 0;

    // Timing
    virtual uint32_t get_ticks_ms() const = 0;
    virtual void delay(uint32_t ms) = 0;
    virtual double get_delta_time() const = 0;

    // Window
    virtual void set_window_title(const std::string& title) = 0;
    virtual Point get_window_size() const = 0;

    // File I/O
    virtual std::vector<uint8_t> read_file(const std::string& path) = 0;
    virtual bool write_file(const std::string& path,
                           const std::vector<uint8_t>& data) = 0;
    virtual bool file_exists(const std::string& path) = 0;

    // Factory
    static std::unique_ptr<Platform> create();
};

// ============================================================
// Renderer Interface
// ============================================================

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool init(Platform& platform, const WindowConfig& config) = 0;
    virtual void shutdown() = 0;

    // Frame management
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void clear(const Color& color) = 0;

    // Texture management
    virtual TextureInfo load_texture(const std::string& path) = 0;
    virtual TextureInfo load_texture_from_memory(const uint8_t* data,
                                                  int width, int height,
                                                  int channels) = 0;
    virtual void destroy_texture(TextureID id) = 0;

    // Drawing primitives
    virtual void draw_texture(TextureID id, const Rect& dest) = 0;
    virtual void draw_texture(TextureID id, const Rect& src,
                              const Rect& dest) = 0;
    virtual void draw_texture_colored(TextureID id, const Rect& src,
                                      const Rect& dest,
                                      const Color& tint) = 0;

    virtual void draw_rect(const Rect& rect, const Color& color) = 0;
    virtual void draw_rect_filled(const Rect& rect, const Color& color) = 0;
    virtual void draw_line(int x1, int y1, int x2, int y2,
                          const Color& color) = 0;

    // Text rendering
    virtual void draw_text(const std::string& text, int x, int y,
                          const Color& color, int size = 12) = 0;

    // Viewport/camera
    virtual void set_viewport(const Rect& viewport) = 0;
    virtual void set_camera(int x, int y) = 0;
    virtual Point screen_to_world(int screen_x, int screen_y) = 0;
    virtual Point world_to_screen(int world_x, int world_y) = 0;

    // Render target scaling
    virtual void set_logical_size(int width, int height) = 0;

    // Factory
    static std::unique_ptr<Renderer> create();
};

// ============================================================
// Audio Interface
// ============================================================

using SoundID = uint32_t;
using MusicID = uint32_t;
constexpr SoundID INVALID_SOUND = 0;
constexpr MusicID INVALID_MUSIC = 0;

class AudioSystem {
public:
    virtual ~AudioSystem() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // Sound effects
    virtual SoundID load_sound(const std::string& path) = 0;
    virtual void play_sound(SoundID id, float volume = 1.0f,
                           float pan = 0.0f) = 0;
    virtual void stop_sound(SoundID id) = 0;

    // Music
    virtual MusicID load_music(const std::string& path) = 0;
    virtual void play_music(MusicID id, bool loop = true) = 0;
    virtual void stop_music() = 0;
    virtual void set_music_volume(float volume) = 0;
    virtual void set_sound_volume(float volume) = 0;

    virtual bool is_music_playing() const = 0;

    // Factory
    static std::unique_ptr<AudioSystem> create();
};

} // namespace CnC
