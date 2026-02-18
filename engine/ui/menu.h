/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Main Menu & UI Screens
 *
 * Menu system for the game with:
 *   - Main menu (campaign, skirmish, options, quit)
 *   - Faction selection
 *   - Skirmish setup (map, difficulty)
 *   - Score screen
 */

#pragma once

#include "../core/platform.h"
#include "../game/game_types.h"
#include <string>
#include <vector>
#include <functional>

namespace CnC {

// ============================================================
// Menu Item
// ============================================================

struct MenuItem {
    std::string label;
    std::string description;
    bool enabled = true;
    bool selected = false;
    Rect bounds;
    int id = 0;
};

// ============================================================
// Menu Screen IDs
// ============================================================

enum class MenuScreen {
    Main,
    FactionSelect,
    SkirmishSetup,
    CampaignMission,
    Options,
    Score
};

// ============================================================
// Skirmish Settings
// ============================================================

struct SkirmishSettings {
    HouseType player_faction = HouseType::GDI;
    int map_size = 1;          // 0=small, 1=medium, 2=large
    int ai_difficulty = 1;     // 0=easy, 1=normal, 2=hard
    int starting_credits = 10000;
    bool fog_of_war = true;
    bool crates = false;

    static const char* map_size_name(int s) {
        switch (s) {
            case 0: return "Small (32x32)";
            case 1: return "Medium (48x48)";
            case 2: return "Large (64x64)";
            default: return "Unknown";
        }
    }

    static const char* difficulty_name(int d) {
        switch (d) {
            case 0: return "Easy";
            case 1: return "Normal";
            case 2: return "Hard";
            default: return "Unknown";
        }
    }
};

// ============================================================
// Main Menu System
// ============================================================

class MenuSystem {
public:
    MenuSystem();

    void init(int screen_width, int screen_height);

    // Input handling
    void handle_event(const InputEvent& event);

    // Update (animations)
    void update(float dt);

    // Rendering
    void render(Renderer& renderer);

    // State
    MenuScreen current_screen() const { return current_screen_; }
    bool should_start_game() const { return start_game_; }
    bool should_quit() const { return quit_; }

    // Result
    SkirmishSettings get_skirmish_settings() const { return skirmish_; }
    bool is_campaign_mode() const { return campaign_mode_; }
    int campaign_mission() const { return campaign_mission_; }
    HouseType campaign_faction() const { return campaign_faction_; }

    // Reset for return to menu
    void reset();

private:
    void build_main_menu();
    void build_faction_select();
    void build_skirmish_setup();
    void build_campaign_select();

    void select_item(int index);
    void confirm_selection();
    void go_back();

    // Rendering helpers
    void render_title(Renderer& renderer);
    void render_menu_items(Renderer& renderer);
    void render_skirmish_setup(Renderer& renderer);
    void render_faction_select(Renderer& renderer);

    // Screen dimensions
    int screen_w_ = 640;
    int screen_h_ = 400;

    // State
    MenuScreen current_screen_ = MenuScreen::Main;
    std::vector<MenuItem> items_;
    int selected_index_ = 0;
    bool start_game_ = false;
    bool quit_ = false;

    // Settings
    SkirmishSettings skirmish_;
    bool campaign_mode_ = false;
    int campaign_mission_ = 1;
    HouseType campaign_faction_ = HouseType::GDI;

    // Animation
    float title_pulse_ = 0.0f;
    float transition_alpha_ = 0.0f;
};

} // namespace CnC
