/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Sidebar UI System
 *
 * The sidebar is the right-panel UI showing:
 *   - Credits/money display
 *   - Power bar
 *   - Minimap
 *   - Build icons (structures, units, infantry)
 *   - Production progress bars
 *   - Superweapon status
 *
 * Original sidebar was 160px wide on a 640-wide screen (25%).
 */

#pragma once

#include "../core/platform.h"
#include "game_types.h"
#include "house.h"
#include "commands.h"
#include <vector>
#include <functional>

namespace CnC {

// ============================================================
// Sidebar Tab
// ============================================================

enum class SidebarTab {
    Structures,
    Units,       // Vehicles + Infantry combined
    Count
};

// ============================================================
// Build Icon
// ============================================================

struct BuildIcon {
    int type_index = -1;
    ProductionCategory category;
    std::string name;
    int cost = 0;
    bool available = true;
    bool building = false;
    float progress = 0.0f;
    bool ready = false;
    Rect bounds;  // Screen position
};

// ============================================================
// Sidebar
// ============================================================

class Sidebar {
public:
    Sidebar();
    ~Sidebar() = default;

    void init(int screen_width, int screen_height);
    void update(float dt, const House& player_house);

    // Rendering
    void render(Renderer& renderer, const House& player_house,
                const TileMap* minimap_source);

    // Input
    bool handle_click(int mouse_x, int mouse_y, MouseButton button);
    bool is_in_sidebar(int mouse_x, int mouse_y) const;

    // Callbacks
    using BuildCallback = std::function<void(ProductionCategory, int)>;
    void set_build_callback(BuildCallback cb) { build_callback_ = cb; }

    // Layout
    Rect get_bounds() const { return sidebar_rect_; }
    int get_width() const { return width_; }

    // Tab control
    void set_tab(SidebarTab tab) { current_tab_ = tab; }
    SidebarTab current_tab() const { return current_tab_; }

private:
    void rebuild_icons(const House& player_house);
    void render_credits(Renderer& renderer, const House& house);
    void render_power_bar(Renderer& renderer, const House& house);
    void render_minimap_frame(Renderer& renderer, const TileMap* map);
    void render_tab_buttons(Renderer& renderer);
    void render_build_icons(Renderer& renderer, const House& house);
    void render_superweapons(Renderer& renderer, const House& house);

    // Layout
    Rect sidebar_rect_;
    int width_ = 160;
    int screen_width_ = 640;
    int screen_height_ = 400;

    // Regions within sidebar
    Rect credits_rect_;
    Rect power_rect_;
    Rect minimap_rect_;
    Rect tab_rect_;
    Rect icons_rect_;
    Rect super_rect_;

    // State
    SidebarTab current_tab_ = SidebarTab::Structures;
    std::vector<BuildIcon> current_icons_;
    int scroll_offset_ = 0;
    int icons_per_row_ = 2;

    // Callback
    BuildCallback build_callback_;
};

// ============================================================
// Game Input Handler
// ============================================================

class GameInputHandler {
public:
    GameInputHandler();
    ~GameInputHandler() = default;

    void init(int screen_width, int screen_height, int sidebar_width);

    // Process an input event - returns true if consumed
    bool handle_event(const InputEvent& event, GameSession& session,
                      Sidebar& sidebar, Renderer& renderer);

    // Per-frame update (scrolling, drag selection)
    void update(float dt, Renderer& renderer);

    // Camera
    int camera_x() const { return camera_x_; }
    int camera_y() const { return camera_y_; }

    // Selection state
    bool is_dragging() const { return drag_selecting_; }
    Rect get_selection_rect() const;

    // Building placement mode
    bool is_placing_building() const { return placing_building_; }
    void start_building_placement(BuildingType type);
    void cancel_building_placement();

private:
    void handle_left_click(int mx, int my, GameSession& session,
                           Sidebar& sidebar, Renderer& renderer);
    void handle_right_click(int mx, int my, GameSession& session,
                            Renderer& renderer);
    void handle_key(KeyCode key, GameSession& session);
    void handle_group_key(int group_num, bool ctrl_held,
                          GameSession& session);
    void update_scroll(float dt);

    // Screen dimensions
    int screen_width_ = 640;
    int screen_height_ = 400;
    int game_area_width_ = 480;  // screen_width - sidebar_width

    // Camera
    int camera_x_ = 0;
    int camera_y_ = 0;
    int scroll_speed_ = 300; // pixels per second
    bool scroll_left_ = false;
    bool scroll_right_ = false;
    bool scroll_up_ = false;
    bool scroll_down_ = false;

    // Edge scrolling
    int edge_scroll_margin_ = 10;
    int mouse_x_ = 0;
    int mouse_y_ = 0;

    // Drag selection
    bool drag_selecting_ = false;
    int drag_start_x_ = 0;
    int drag_start_y_ = 0;
    int drag_end_x_ = 0;
    int drag_end_y_ = 0;

    // Building placement
    bool placing_building_ = false;
    BuildingType placing_type_ = BuildingType::None;
    int placement_cell_x_ = -1;
    int placement_cell_y_ = -1;

    // Unit groups
    UnitGroupManager unit_groups_;

    // Command feedback
    CommandFeedback command_feedback_;

public:
    // Access to subsystems
    UnitGroupManager& unit_groups() { return unit_groups_; }
    CommandFeedback& feedback() { return command_feedback_; }
};

} // namespace CnC
