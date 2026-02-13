/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * SDL2 Platform Implementation
 *
 * Desktop platform backend using SDL2 for window management,
 * input handling, and timing. Works on Windows, macOS, and Linux.
 */

#include "platform.h"

#ifndef PLATFORM_WEB

#include <SDL2/SDL.h>
#include <fstream>
#include <queue>

namespace CnC {

class PlatformSDL : public Platform {
public:
    ~PlatformSDL() override { shutdown(); }

    bool init(const WindowConfig& config) override {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
            return false;
        }

        uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        if (config.fullscreen) {
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }

        window_ = SDL_CreateWindow(
            config.title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            config.width, config.height,
            flags
        );

        if (!window_) {
            SDL_Quit();
            return false;
        }

        config_ = config;
        quit_ = false;
        return true;
    }

    void shutdown() override {
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
        SDL_Quit();
    }

    bool should_quit() const override {
        return quit_;
    }

    void poll_events() override {
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event)) {
            InputEvent event{};

            switch (sdl_event.type) {
                case SDL_QUIT:
                    event.type = InputEvent::Type::Quit;
                    quit_ = true;
                    event_queue_.push(event);
                    break;

                case SDL_MOUSEMOTION:
                    event.type = InputEvent::Type::MouseMove;
                    event.mouse_x = sdl_event.motion.x;
                    event.mouse_y = sdl_event.motion.y;
                    event_queue_.push(event);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    event.type = InputEvent::Type::MouseDown;
                    event.mouse_x = sdl_event.button.x;
                    event.mouse_y = sdl_event.button.y;
                    event.button = map_mouse_button(sdl_event.button.button);
                    event_queue_.push(event);
                    break;

                case SDL_MOUSEBUTTONUP:
                    event.type = InputEvent::Type::MouseUp;
                    event.mouse_x = sdl_event.button.x;
                    event.mouse_y = sdl_event.button.y;
                    event.button = map_mouse_button(sdl_event.button.button);
                    event_queue_.push(event);
                    break;

                case SDL_MOUSEWHEEL:
                    event.type = InputEvent::Type::MouseWheel;
                    event.wheel_delta = sdl_event.wheel.y;
                    event_queue_.push(event);
                    break;

                case SDL_KEYDOWN:
                    if (!sdl_event.key.repeat) {
                        event.type = InputEvent::Type::KeyDown;
                        event.key = map_key(sdl_event.key.keysym.sym);
                        event.shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
                        event.ctrl = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
                        event.alt = (sdl_event.key.keysym.mod & KMOD_ALT) != 0;
                        event_queue_.push(event);
                    }
                    break;

                case SDL_KEYUP:
                    event.type = InputEvent::Type::KeyUp;
                    event.key = map_key(sdl_event.key.keysym.sym);
                    event_queue_.push(event);
                    break;

                case SDL_WINDOWEVENT:
                    if (sdl_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        event.type = InputEvent::Type::WindowResize;
                        event.mouse_x = sdl_event.window.data1; // width
                        event.mouse_y = sdl_event.window.data2; // height
                        event_queue_.push(event);
                    }
                    break;
            }
        }
    }

    bool pop_event(InputEvent& event) override {
        if (event_queue_.empty()) return false;
        event = event_queue_.front();
        event_queue_.pop();
        return true;
    }

    uint32_t get_ticks_ms() const override {
        return SDL_GetTicks();
    }

    void delay(uint32_t ms) override {
        SDL_Delay(ms);
    }

    double get_delta_time() const override {
        static uint64_t last = SDL_GetPerformanceCounter();
        uint64_t now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / SDL_GetPerformanceFrequency();
        last = now;
        return dt;
    }

    void set_window_title(const std::string& title) override {
        if (window_) SDL_SetWindowTitle(window_, title.c_str());
    }

    Point get_window_size() const override {
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        return Point(w, h);
    }

    std::vector<uint8_t> read_file(const std::string& path) override {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return {};

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        return buffer;
    }

    bool write_file(const std::string& path,
                   const std::vector<uint8_t>& data) override {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return file.good();
    }

    bool file_exists(const std::string& path) override {
        std::ifstream file(path);
        return file.good();
    }

    SDL_Window* get_window() const { return window_; }

private:
    static MouseButton map_mouse_button(uint8_t sdl_button) {
        switch (sdl_button) {
            case SDL_BUTTON_LEFT:   return MouseButton::Left;
            case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
            case SDL_BUTTON_RIGHT:  return MouseButton::Right;
            default:                return MouseButton::Left;
        }
    }

    static KeyCode map_key(SDL_Keycode key) {
        switch (key) {
            case SDLK_a: return KeyCode::A;
            case SDLK_b: return KeyCode::B;
            case SDLK_c: return KeyCode::C;
            case SDLK_d: return KeyCode::D;
            case SDLK_e: return KeyCode::E;
            case SDLK_f: return KeyCode::F;
            case SDLK_g: return KeyCode::G;
            case SDLK_h: return KeyCode::H;
            case SDLK_i: return KeyCode::I;
            case SDLK_j: return KeyCode::J;
            case SDLK_k: return KeyCode::K;
            case SDLK_l: return KeyCode::L;
            case SDLK_m: return KeyCode::M;
            case SDLK_n: return KeyCode::N;
            case SDLK_o: return KeyCode::O;
            case SDLK_p: return KeyCode::P;
            case SDLK_q: return KeyCode::Q;
            case SDLK_r: return KeyCode::R;
            case SDLK_s: return KeyCode::S;
            case SDLK_t: return KeyCode::T;
            case SDLK_u: return KeyCode::U;
            case SDLK_v: return KeyCode::V;
            case SDLK_w: return KeyCode::W;
            case SDLK_x: return KeyCode::X;
            case SDLK_y: return KeyCode::Y;
            case SDLK_z: return KeyCode::Z;
            case SDLK_0: return KeyCode::Num0;
            case SDLK_1: return KeyCode::Num1;
            case SDLK_2: return KeyCode::Num2;
            case SDLK_3: return KeyCode::Num3;
            case SDLK_4: return KeyCode::Num4;
            case SDLK_5: return KeyCode::Num5;
            case SDLK_6: return KeyCode::Num6;
            case SDLK_7: return KeyCode::Num7;
            case SDLK_8: return KeyCode::Num8;
            case SDLK_9: return KeyCode::Num9;
            case SDLK_ESCAPE:    return KeyCode::Escape;
            case SDLK_RETURN:    return KeyCode::Enter;
            case SDLK_SPACE:     return KeyCode::Space;
            case SDLK_TAB:       return KeyCode::Tab;
            case SDLK_BACKSPACE: return KeyCode::Backspace;
            case SDLK_LEFT:      return KeyCode::Left;
            case SDLK_RIGHT:     return KeyCode::Right;
            case SDLK_UP:        return KeyCode::Up;
            case SDLK_DOWN:      return KeyCode::Down;
            case SDLK_F1:  return KeyCode::F1;
            case SDLK_F2:  return KeyCode::F2;
            case SDLK_F3:  return KeyCode::F3;
            case SDLK_F4:  return KeyCode::F4;
            case SDLK_F5:  return KeyCode::F5;
            case SDLK_F6:  return KeyCode::F6;
            case SDLK_F7:  return KeyCode::F7;
            case SDLK_F8:  return KeyCode::F8;
            case SDLK_F9:  return KeyCode::F9;
            case SDLK_F10: return KeyCode::F10;
            case SDLK_F11: return KeyCode::F11;
            case SDLK_F12: return KeyCode::F12;
            case SDLK_LSHIFT: return KeyCode::LShift;
            case SDLK_RSHIFT: return KeyCode::RShift;
            case SDLK_LCTRL:  return KeyCode::LCtrl;
            case SDLK_RCTRL:  return KeyCode::RCtrl;
            case SDLK_LALT:   return KeyCode::LAlt;
            case SDLK_RALT:   return KeyCode::RAlt;
            case SDLK_DELETE:   return KeyCode::Delete;
            case SDLK_HOME:     return KeyCode::Home;
            case SDLK_END:      return KeyCode::End;
            case SDLK_PAGEUP:   return KeyCode::PageUp;
            case SDLK_PAGEDOWN: return KeyCode::PageDown;
            default: return KeyCode::Unknown;
        }
    }

    SDL_Window* window_ = nullptr;
    WindowConfig config_;
    bool quit_ = false;
    std::queue<InputEvent> event_queue_;
};

// Factory
std::unique_ptr<Platform> Platform::create() {
    return std::make_unique<PlatformSDL>();
}

} // namespace CnC

#endif // !PLATFORM_WEB
