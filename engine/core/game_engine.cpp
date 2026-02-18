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

    // Initialize game systems
    session_ = std::make_unique<GameSession>();
    sidebar_ = std::make_unique<Sidebar>();
    input_handler_ = std::make_unique<GameInputHandler>();
    ai_manager_ = std::make_unique<AIManager>();
    scenario_loader_ = std::make_unique<ScenarioLoader>();

    // Initialize sidebar UI
    sidebar_->init(config_.window.logical_width, config_.window.logical_height);

    // Initialize input handler
    input_handler_->init(config_.window.logical_width,
                         config_.window.logical_height,
                         sidebar_->get_width());

    // Wire up sidebar build callbacks to the session
    sidebar_->set_build_callback(
        [this](ProductionCategory cat, int type_index) {
            if (session_) {
                House& player = session_->player_house();
                player.start_building(cat, type_index);
            }
        }
    );

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

    ai_manager_.reset();
    session_.reset();
    sidebar_.reset();
    input_handler_.reset();
    scenario_loader_.reset();

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
                // Let the input handler process gameplay events first
                if (input_handler_ && session_ && sidebar_) {
                    if (input_handler_->handle_event(event, *session_,
                                                      *sidebar_, *renderer_)) {
                        break; // Event consumed by gameplay handler
                    }
                }
                // Unhandled gameplay keys
                if (event.type == InputEvent::Type::KeyDown) {
                    if (event.key == KeyCode::Escape) {
                        // Cancel building placement first, or pause
                        if (input_handler_ && input_handler_->is_placing_building()) {
                            input_handler_->cancel_building_placement();
                        } else {
                            set_state(GameState::Paused);
                        }
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
    // Load the default skirmish scenario
    current_scenario_ = ScenarioLibrary::skirmish_small();

    // Connect session to tilemap
    session_->set_tilemap(tilemap_.get());

    // Load the scenario into the game
    scenario_loader_->load(current_scenario_, *session_, *tilemap_);

    // Setup AI players for non-human houses
    ai_manager_->clear();
    for (const auto& sp : current_scenario_.players) {
        if (!sp.is_human) {
            ai_manager_->add_ai(sp.house, AIDifficulty::Normal,
                               sp.base_cell_x, sp.base_cell_y);
        }
    }

    set_state(GameState::Playing);
}

void GameEngine::update_playing(float dt) {
    // Update input handler (camera scrolling, edge scroll)
    if (input_handler_) {
        input_handler_->update(dt, *renderer_);
    }

    // Accumulate time for fixed game tick updates
    // Original C&C runs game logic at ~15 ticks per second
    tick_accumulator_ += dt;

    while (tick_accumulator_ >= GAME_TICK_RATE) {
        tick_accumulator_ -= GAME_TICK_RATE;
        game_tick_++;

        // Update game session (entities, combat, economy, production)
        if (session_) {
            session_->update(GAME_TICK_RATE);
        }

        // Update AI players
        if (ai_manager_ && session_) {
            ai_manager_->update(GAME_TICK_RATE, *session_, *tilemap_);
        }

        // Tiberium growth happens every ~300 ticks
        if (game_tick_ % 300 == 0) {
            tilemap_->spread_tiberium();
        }

        // Check for game over
        if (session_ && session_->is_game_over()) {
            set_state(GameState::ScenarioComplete);
        }
    }

    // Update sidebar (continuous for animations)
    if (sidebar_ && session_) {
        sidebar_->update(dt, session_->player_house());
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

        case GameState::ScenarioComplete:
            render_playing();
            // Victory/defeat overlay
            if (session_) {
                int cx = config_.window.logical_width / 2;
                int cy = config_.window.logical_height / 2;
                bool won = session_->winner() ==
                           session_->player_house().type();
                renderer_->draw_text(
                    won ? "MISSION ACCOMPLISHED" : "MISSION FAILED",
                    cx - 90, cy - 10,
                    won ? Color::GDI() : Color::Nod(), 18);
                renderer_->draw_text("Press ESC to return to menu",
                    cx - 100, cy + 20, Color::White(), 10);
            }
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
    int cam_x = input_handler_ ? input_handler_->camera_x() : 0;
    int cam_y = input_handler_ ? input_handler_->camera_y() : 0;
    int game_w = config_.window.logical_width - (sidebar_ ? sidebar_->get_width() : 0);
    int game_h = config_.window.logical_height;

    Rect viewport(cam_x, cam_y, game_w, game_h);

    // Set clipping to game area (exclude sidebar)
    renderer_->set_clip_rect(Rect(0, 0, game_w, game_h));

    // 1. Terrain tiles
    tilemap_->render(*renderer_, viewport);

    // 2. Buildings (below units, sorted by Y)
    if (session_) {
        session_->entities().render(*renderer_, viewport);
    }

    // 3. Fog of war overlay
    if (config_.fog_of_war) {
        tilemap_->render_fog(*renderer_, viewport);
    }

    // 4. Selection boxes and health bars
    if (session_) {
        session_->entities().render_selection_boxes(*renderer_, viewport);
        session_->entities().render_health_bars(*renderer_, viewport);
    }

    // 5. Drag selection rectangle
    if (input_handler_ && input_handler_->is_dragging()) {
        Rect sel = input_handler_->get_selection_rect();
        renderer_->draw_rect(sel, Color(0, 255, 0, 180));
    }

    // Clear clipping for sidebar
    renderer_->set_clip_rect(Rect(0, 0, config_.window.logical_width,
                                  config_.window.logical_height));

    // 6. Sidebar UI
    if (sidebar_ && session_) {
        sidebar_->render(*renderer_, session_->player_house(), tilemap_.get());
    }

    // 7. Game info overlay
    if (session_) {
        std::string tick_str = "Tick: " + std::to_string(game_tick_);
        renderer_->draw_text(tick_str, game_w - 80, game_h - 16,
                            Color::White(), 8);
    }

    // Pause overlay
    if (current_state_ == GameState::Paused) {
        // Dim overlay
        Rect full(0, 0, config_.window.logical_width,
                  config_.window.logical_height);
        renderer_->draw_rect_filled(full, Color(0, 0, 0, 120));
        renderer_->draw_text("PAUSED", config_.window.logical_width / 2 - 30,
                            config_.window.logical_height / 2 - 10,
                            Color::Yellow(), 16);
        renderer_->draw_text("Press ESC to resume",
                            config_.window.logical_width / 2 - 65,
                            config_.window.logical_height / 2 + 15,
                            Color::White(), 10);
    }
}

} // namespace CnC
