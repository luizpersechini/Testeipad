/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Main Game Engine
 *
 * Orchestrates all game subsystems and manages the main game loop.
 * Handles state transitions (menu, gameplay, cinematics).
 */

#pragma once

#include "platform.h"
#include "../graphics/sprite.h"
#include "../graphics/tilemap.h"
#include "../game/game_types.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

namespace CnC {

// Forward declarations
class GameMap;
class GameSession;
class Sidebar;
class UIManager;

// ============================================================
// Game State Machine
// ============================================================

enum class GameState {
    Startup,
    MainMenu,
    Loading,
    Playing,
    Paused,
    ScenarioComplete,
    GameOver,
    Cinematic,
    MapEditor,
    Shutdown
};

// ============================================================
// Game Configuration
// ============================================================

struct GameConfig {
    WindowConfig window;

    // Game settings
    int game_speed = 5;        // 0-7, matching original
    int scroll_speed = 5;      // Map scroll speed
    float music_volume = 0.7f;
    float sfx_volume = 1.0f;
    bool fog_of_war = true;

    // Rendering
    bool smooth_scaling = true;
    bool show_fps = false;

    // Asset paths
    std::string asset_base_path = "assets/";
    std::string sprite_path = "assets/sprites/";
    std::string tile_path = "assets/tiles/";
    std::string sound_path = "assets/sounds/";
    std::string music_path = "assets/music/";
};

// ============================================================
// Main Engine Class
// ============================================================

class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    // Lifecycle
    bool init(const GameConfig& config);
    void run();
    void shutdown();

    // State management
    void set_state(GameState new_state);
    GameState get_state() const { return current_state_; }

    // Subsystem access
    Platform& platform() { return *platform_; }
    Renderer& renderer() { return *renderer_; }
    AudioSystem& audio() { return *audio_; }
    SpriteManager& sprites() { return *sprites_; }
    TileMap& tilemap() { return *tilemap_; }

    // Game time
    uint32_t get_game_tick() const { return game_tick_; }
    float get_game_time() const { return game_time_; }
    float get_delta_time() const { return delta_time_; }
    int get_fps() const { return fps_; }

    // Configuration
    const GameConfig& config() const { return config_; }
    GameConfig& mutable_config() { return config_; }

    // Singleton access (single game instance)
    static GameEngine& instance();

private:
    // Main loop phases
    void process_input();
    void update(float dt);
    void render();

    // State-specific update/render
    void update_main_menu(float dt);
    void render_main_menu();
    void update_playing(float dt);
    void render_playing();
    void update_loading(float dt);
    void render_loading();

    // Timing
    void update_timing();

    // Subsystems
    std::unique_ptr<Platform> platform_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<AudioSystem> audio_;
    std::unique_ptr<SpriteManager> sprites_;
    std::unique_ptr<TileMap> tilemap_;

    // Game state
    GameState current_state_ = GameState::Startup;
    GameConfig config_;

    // Timing
    uint32_t game_tick_ = 0;
    float game_time_ = 0.0f;
    float delta_time_ = 0.0f;
    uint32_t last_tick_ = 0;
    int fps_ = 0;
    int frame_count_ = 0;
    uint32_t fps_timer_ = 0;

    // Game speed timing (original C&C ran at ~15 FPS game logic)
    static constexpr float GAME_TICK_RATE = 1.0f / 15.0f;
    float tick_accumulator_ = 0.0f;

    bool running_ = false;

    static GameEngine* instance_;
};

} // namespace CnC
