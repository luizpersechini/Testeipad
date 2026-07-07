/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Emscripten/Web Platform Implementation
 *
 * Web platform backend using Emscripten for browser deployment.
 * Uses HTML5 Canvas/WebGL for rendering and Web Audio for sound.
 */

#include "platform.h"

#ifdef PLATFORM_WEB

#include <emscripten.h>
#include <emscripten/html5.h>
#include <queue>
#include <fstream>

namespace CnC {

class PlatformWeb : public Platform {
public:
    ~PlatformWeb() override { shutdown(); }

    bool init(const WindowConfig& config) override {
        config_ = config;

        // Set canvas size
        emscripten_set_canvas_element_size("#canvas",
                                           config.width, config.height);

        // Register input callbacks
        emscripten_set_mousemove_callback("#canvas", this, true,
                                          on_mouse_move);
        emscripten_set_mousedown_callback("#canvas", this, true,
                                           on_mouse_down);
        emscripten_set_mouseup_callback("#canvas", this, true,
                                         on_mouse_up);
        emscripten_set_wheel_callback("#canvas", this, true,
                                       on_wheel);
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                        this, true, on_key_down);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                      this, true, on_key_up);
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,
                                       this, true, on_resize);

        quit_ = false;
        return true;
    }

    void shutdown() override {
        // Clean up callbacks
    }

    bool should_quit() const override { return quit_; }

    void poll_events() override {
        // Events are pushed via callbacks in Emscripten
    }

    bool pop_event(InputEvent& event) override {
        if (event_queue_.empty()) return false;
        event = event_queue_.front();
        event_queue_.pop();
        return true;
    }

    uint32_t get_ticks_ms() const override {
        return static_cast<uint32_t>(emscripten_get_now());
    }

    void delay(uint32_t ms) override {
        emscripten_sleep(ms);
    }

    double get_delta_time() const override {
        static double last_time = emscripten_get_now();
        double now = emscripten_get_now();
        double dt = (now - last_time) / 1000.0;
        last_time = now;
        return dt;
    }

    void set_window_title(const std::string& title) override {
        EM_ASM({ document.title = UTF8ToString($0); }, title.c_str());
    }

    Point get_window_size() const override {
        int w, h;
        emscripten_get_canvas_element_size("#canvas", &w, &h);
        return Point(w, h);
    }

    std::vector<uint8_t> read_file(const std::string& path) override {
        // Emscripten virtual filesystem
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

private:
    // Emscripten callback handlers
    static EM_BOOL on_mouse_move(int event_type,
                                  const EmscriptenMouseEvent* e,
                                  void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::MouseMove;
        event.mouse_x = e->targetX;
        event.mouse_y = e->targetY;
        self->event_queue_.push(event);
        return EM_TRUE;
    }

    static EM_BOOL on_mouse_down(int event_type,
                                  const EmscriptenMouseEvent* e,
                                  void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::MouseDown;
        event.mouse_x = e->targetX;
        event.mouse_y = e->targetY;
        event.button = static_cast<MouseButton>(e->button);
        self->event_queue_.push(event);
        return EM_TRUE;
    }

    static EM_BOOL on_mouse_up(int event_type,
                                const EmscriptenMouseEvent* e,
                                void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::MouseUp;
        event.mouse_x = e->targetX;
        event.mouse_y = e->targetY;
        event.button = static_cast<MouseButton>(e->button);
        self->event_queue_.push(event);
        return EM_TRUE;
    }

    static EM_BOOL on_wheel(int event_type,
                             const EmscriptenWheelEvent* e,
                             void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::MouseWheel;
        event.wheel_delta = static_cast<int>(-e->deltaY);
        self->event_queue_.push(event);
        return EM_TRUE;
    }

    static EM_BOOL on_key_down(int event_type,
                                const EmscriptenKeyboardEvent* e,
                                void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::KeyDown;
        event.key = map_web_key(e->code);
        event.shift = e->shiftKey;
        event.ctrl = e->ctrlKey;
        event.alt = e->altKey;
        self->event_queue_.push(event);

        // Prevent default for game keys (arrows, space, etc.)
        std::string code(e->code);
        if (code.find("Arrow") != std::string::npos ||
            code == "Space" || code == "Tab") {
            return EM_TRUE; // preventDefault
        }
        return EM_FALSE;
    }

    static EM_BOOL on_key_up(int event_type,
                              const EmscriptenKeyboardEvent* e,
                              void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::KeyUp;
        event.key = map_web_key(e->code);
        self->event_queue_.push(event);
        return EM_FALSE;
    }

    static EM_BOOL on_resize(int event_type,
                              const EmscriptenUiEvent* e,
                              void* user_data) {
        auto* self = static_cast<PlatformWeb*>(user_data);
        InputEvent event{};
        event.type = InputEvent::Type::WindowResize;
        event.mouse_x = e->windowInnerWidth;
        event.mouse_y = e->windowInnerHeight;
        self->event_queue_.push(event);
        return EM_TRUE;
    }

    static KeyCode map_web_key(const char* code) {
        std::string key(code);
        if (key == "KeyA") return KeyCode::A;
        if (key == "KeyB") return KeyCode::B;
        if (key == "KeyC") return KeyCode::C;
        if (key == "KeyD") return KeyCode::D;
        if (key == "KeyE") return KeyCode::E;
        if (key == "KeyF") return KeyCode::F;
        if (key == "KeyG") return KeyCode::G;
        if (key == "KeyH") return KeyCode::H;
        if (key == "KeyI") return KeyCode::I;
        if (key == "KeyJ") return KeyCode::J;
        if (key == "KeyK") return KeyCode::K;
        if (key == "KeyL") return KeyCode::L;
        if (key == "KeyM") return KeyCode::M;
        if (key == "KeyN") return KeyCode::N;
        if (key == "KeyO") return KeyCode::O;
        if (key == "KeyP") return KeyCode::P;
        if (key == "KeyQ") return KeyCode::Q;
        if (key == "KeyR") return KeyCode::R;
        if (key == "KeyS") return KeyCode::S;
        if (key == "KeyT") return KeyCode::T;
        if (key == "KeyU") return KeyCode::U;
        if (key == "KeyV") return KeyCode::V;
        if (key == "KeyW") return KeyCode::W;
        if (key == "KeyX") return KeyCode::X;
        if (key == "KeyY") return KeyCode::Y;
        if (key == "KeyZ") return KeyCode::Z;
        if (key == "Digit0") return KeyCode::Num0;
        if (key == "Digit1") return KeyCode::Num1;
        if (key == "Digit2") return KeyCode::Num2;
        if (key == "Digit3") return KeyCode::Num3;
        if (key == "Digit4") return KeyCode::Num4;
        if (key == "Digit5") return KeyCode::Num5;
        if (key == "Digit6") return KeyCode::Num6;
        if (key == "Digit7") return KeyCode::Num7;
        if (key == "Digit8") return KeyCode::Num8;
        if (key == "Digit9") return KeyCode::Num9;
        if (key == "Escape")     return KeyCode::Escape;
        if (key == "Enter")      return KeyCode::Enter;
        if (key == "Space")      return KeyCode::Space;
        if (key == "Tab")        return KeyCode::Tab;
        if (key == "Backspace")  return KeyCode::Backspace;
        if (key == "ArrowLeft")  return KeyCode::Left;
        if (key == "ArrowRight") return KeyCode::Right;
        if (key == "ArrowUp")    return KeyCode::Up;
        if (key == "ArrowDown")  return KeyCode::Down;
        if (key == "F1")  return KeyCode::F1;
        if (key == "F2")  return KeyCode::F2;
        if (key == "F3")  return KeyCode::F3;
        if (key == "F4")  return KeyCode::F4;
        if (key == "F5")  return KeyCode::F5;
        if (key == "F6")  return KeyCode::F6;
        if (key == "F7")  return KeyCode::F7;
        if (key == "F8")  return KeyCode::F8;
        if (key == "F9")  return KeyCode::F9;
        if (key == "F10") return KeyCode::F10;
        if (key == "F11") return KeyCode::F11;
        if (key == "F12") return KeyCode::F12;
        if (key == "ShiftLeft")    return KeyCode::LShift;
        if (key == "ShiftRight")   return KeyCode::RShift;
        if (key == "ControlLeft")  return KeyCode::LCtrl;
        if (key == "ControlRight") return KeyCode::RCtrl;
        if (key == "AltLeft")      return KeyCode::LAlt;
        if (key == "AltRight")     return KeyCode::RAlt;
        if (key == "Delete")       return KeyCode::Delete;
        if (key == "Home")         return KeyCode::Home;
        if (key == "End")          return KeyCode::End;
        if (key == "PageUp")       return KeyCode::PageUp;
        if (key == "PageDown")     return KeyCode::PageDown;
        return KeyCode::Unknown;
    }

    WindowConfig config_;
    bool quit_ = false;
    std::queue<InputEvent> event_queue_;
};

// Factory
std::unique_ptr<Platform> Platform::create() {
    return std::make_unique<PlatformWeb>();
}

} // namespace CnC

#endif // PLATFORM_WEB
