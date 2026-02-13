/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Main Game Engine Implementation
 */

#include "game_engine.h"
#include <algorithm>

#ifdef PLATFORM_WEB
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace CnC {

GameEngine* GameEngine::instance_ = nullptr;

GameEngine::GameEngine() {
    instance_ = this;
}

GameEngine::~GameEngine() {
    if (instance_ == this) instance_ = nullptr;
}

GameEngine& GameEngine::instance() {
    return *instance_;
}

bool GameEngine::init(const GameConfig& config) {
    config_ = config;

    // Create and initialize platform
    platform_ = Platform::create();
    if (!platform_ || !platform_->init(config_.window)) {
        return false;
    }

    // Create and initialize renderer
    renderer_ = Renderer::create();
    if (!renderer_ || !renderer_->init(*platform_, config_.window)) {
        return false;
    }
    renderer_->set_logical_size(config_.window.logical_width,
                                config_.window.logical_height);

    // Create and initialize audio
    audio_ = AudioSystem::create();
    if (audio_) {
        audio_->init();
        audio_->set_music_volume(config_.music_volume);
        audio_->set_sound_volume(config_.sfx_volume);
    }

    // Initialize sprite manager
    sprites_ = std::make_unique<SpriteManager>();
    sprites_->init(*renderer_, config_.sprite_path);

    // Initialize tile map
    tilemap_ = std::make_unique<TileMap>();
    tilemap_->init(*renderer_, config_.tile_path);

    // Initialize game data registry
    GameDataRegistry::instance().init();

    // Initialize timing
    last_tick_ = platform_->get_ticks_ms();
    fps_timer_ = last_tick_;

    current_state_ = GameState::MainMenu;
    running_ = true;

    return true;
}

void GameEngine::run() {
#ifdef PLATFORM_WEB
    // Emscripten uses a callback-based main loop
    emscripten_set_main_loop_arg(
        [](void* arg) {
            auto* engine = static_cast<GameEngine*>(arg);
            engine->update_timing();
            engine->process_input();
            engine->update(engine->delta_time_);
            engine->render();
        },
        this, 0, 1
    );
#else
    // Desktop main loop
    while (running_ && !platform_->should_quit()) {
        update_timing();
        process_input();
        update(delta_time_);
        render();
    }
#endif
}

void GameEngine::shutdown() {
    running_ = false;

    if (tilemap_) tilemap_->shutdown();
    if (sprites_) sprites_->shutdown();
    if (audio_) audio_->shutdown();
    if (renderer_) renderer_->shutdown();
    if (platform_) platform_->shutdown();
}

void GameEngine::set_state(GameState new_state) {
    current_state_ = new_state;
}

void GameEngine::update_timing() {
    uint32_t now = platform_->get_ticks_ms();
    delta_time_ = (now - last_tick_) / 1000.0f;
    last_tick_ = now;

    // Clamp delta time to avoid spiral of death
    if (delta_time_ > 0.1f) delta_time_ = 0.1f;

    // FPS counter
    frame_count_++;
    if (now - fps_timer_ >= 1000) {
        fps_ = frame_count_;
        frame_count_ = 0;
        fps_timer_ = now;
    }
}

void GameEngine::process_input() {
    platform_->poll_events();

    InputEvent event;
    while (platform_->pop_event(event)) {
        if (event.type == InputEvent::Type::Quit) {
            running_ = false;
            return;
        }

        switch (current_state_) {
            case GameState::MainMenu:
                // Handle menu input
                if (event.type == InputEvent::Type::KeyDown) {
                    if (event.key == KeyCode::Enter ||
                        event.key == KeyCode::Space) {
                        set_state(GameState::Loading);
                    }
                    if (event.key == KeyCode::Escape) {
                        running_ = false;
                    }
                }
                break;

            case GameState::Playing:
                // Handle gameplay input (scrolling, selection, commands)
                if (event.type == InputEvent::Type::KeyDown) {
                    if (event.key == KeyCode::Escape) {
                        set_state(GameState::Paused);
                    }
                }
                break;

            case GameState::Paused:
                if (event.type == InputEvent::Type::KeyDown) {
                    if (event.key == KeyCode::Escape) {
                        set_state(GameState::Playing);
                    }
                }
                break;

            default:
                break;
        }
    }
}

void GameEngine::update(float dt) {
    game_time_ += dt;

    switch (current_state_) {
        case GameState::MainMenu:
            update_main_menu(dt);
            break;

        case GameState::Loading:
            update_loading(dt);
            break;

        case GameState::Playing:
            update_playing(dt);
            break;

        case GameState::Paused:
            // No game logic updates when paused
            break;

        default:
            break;
    }
}

void GameEngine::update_main_menu(float /*dt*/) {
    // Animate menu elements, handle menu logic
}

void GameEngine::update_loading(float /*dt*/) {
    // Load scenario, generate map, load assets
    // For now, transition directly to playing with a test map
    tilemap_->generate_test_map(32, 32);
    set_state(GameState::Playing);
}

void GameEngine::update_playing(float dt) {
    // Accumulate time for fixed game tick updates
    // Original C&C runs game logic at ~15 ticks per second
    tick_accumulator_ += dt;

    while (tick_accumulator_ >= GAME_TICK_RATE) {
        tick_accumulator_ -= GAME_TICK_RATE;
        game_tick_++;

        // Fixed-rate game logic updates:
        // - Unit movement and pathfinding
        // - Combat resolution
        // - AI decisions
        // - Tiberium growth
        // - Building production
        // - Trigger/event checks

        // Tiberium growth happens every ~300 ticks
        if (game_tick_ % 300 == 0) {
            tilemap_->spread_tiberium();
        }
    }
}

void GameEngine::render() {
    renderer_->begin_frame();
    renderer_->clear(Color::Black());

    switch (current_state_) {
        case GameState::MainMenu:
            render_main_menu();
            break;

        case GameState::Loading:
            render_loading();
            break;

        case GameState::Playing:
        case GameState::Paused:
            render_playing();
            break;

        default:
            break;
    }

    // FPS counter overlay
    if (config_.show_fps) {
        renderer_->draw_text("FPS: " + std::to_string(fps_),
                            4, 4, Color::Yellow(), 10);
    }

    renderer_->end_frame();
}

void GameEngine::render_main_menu() {
    // Render main menu
    int cx = config_.window.logical_width / 2;
    int cy = config_.window.logical_height / 2;

    renderer_->draw_text("COMMAND & CONQUER", cx - 100, cy - 60,
                         Color::Yellow(), 20);
    renderer_->draw_text("TIBERIAN DAWN", cx - 75, cy - 30,
                         Color::GDI(), 16);
    renderer_->draw_text("Modern Port", cx - 50, cy, Color::White(), 12);
    renderer_->draw_text("Press ENTER to start", cx - 80, cy + 40,
                         Color::White(), 10);
    renderer_->draw_text("Press ESC to quit", cx - 65, cy + 60,
                         Color::White(), 10);
}

void GameEngine::render_loading() {
    int cx = config_.window.logical_width / 2;
    int cy = config_.window.logical_height / 2;

    renderer_->draw_text("Loading...", cx - 40, cy, Color::White(), 14);
}

void GameEngine::render_playing() {
    // Calculate viewport based on camera position
    Rect viewport(0, 0,
                  config_.window.logical_width,
                  config_.window.logical_height);

    // Render layers in order:
    // 1. Terrain tiles
    tilemap_->render(*renderer_, viewport);

    // 2. Building sprites (below units)
    // 3. Infantry and vehicle sprites
    // 4. Aircraft sprites
    // 5. Projectiles and effects
    // 6. Fog of war overlay
    tilemap_->render_fog(*renderer_, viewport);

    // 7. Sidebar UI
    // 8. Selection indicators
    // 9. Minimap

    // Draw minimap in bottom-right corner
    Rect minimap_rect(config_.window.logical_width - 130,
                      config_.window.logical_height - 130,
                      120, 120);
    renderer_->draw_rect_filled(minimap_rect, Color(0, 0, 0, 180));
    tilemap_->render_minimap(*renderer_, minimap_rect);
    renderer_->draw_rect(minimap_rect, Color::White());

    if (current_state_ == GameState::Paused) {
        renderer_->draw_text("PAUSED", config_.window.logical_width / 2 - 30,
                            config_.window.logical_height / 2,
                            Color::Yellow(), 16);
    }
}

} // namespace CnC
