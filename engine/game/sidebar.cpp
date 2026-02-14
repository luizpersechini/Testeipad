/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Sidebar UI & Input Handler Implementation
 */

#include "sidebar.h"
#include <algorithm>
#include <cmath>

namespace CnC {

// ============================================================
// Sidebar
// ============================================================

Sidebar::Sidebar() = default;

void Sidebar::init(int screen_width, int screen_height) {
    screen_width_ = screen_width;
    screen_height_ = screen_height;

    // Sidebar occupies the right 160 pixels
    width_ = 160;
    sidebar_rect_ = Rect(screen_width - width_, 0, width_, screen_height);

    // Layout regions within sidebar
    int x = sidebar_rect_.x;
    int y = 0;

    credits_rect_ = Rect(x, y, width_, 20);
    y += 22;

    power_rect_ = Rect(x + 4, y, 12, screen_height / 3);
    minimap_rect_ = Rect(x + 20, y, width_ - 24, width_ - 24);
    y += std::max(power_rect_.h, minimap_rect_.h) + 4;

    tab_rect_ = Rect(x, y, width_, 22);
    y += 24;

    icons_rect_ = Rect(x, y, width_, screen_height - y - 30);
    super_rect_ = Rect(x, screen_height - 28, width_, 26);
}

void Sidebar::update(float dt, const House& player_house) {
    rebuild_icons(player_house);
}

void Sidebar::rebuild_icons(const House& player_house) {
    current_icons_.clear();

    const auto& registry = GameDataRegistry::instance();

    if (current_tab_ == SidebarTab::Structures) {
        auto available = player_house.available_structures();
        for (BuildingType bt : available) {
            BuildIcon icon;
            icon.type_index = static_cast<int>(bt);
            icon.category = ProductionCategory::Structure;
            icon.name = building_name(bt);
            icon.cost = registry.get_building(bt).cost;
            icon.available = true;

            const auto* prod = player_house.current_production(
                ProductionCategory::Structure);
            if (prod && prod->type_index == icon.type_index) {
                icon.building = true;
                icon.progress = prod->progress;
                icon.ready = prod->ready;
            }

            current_icons_.push_back(icon);
        }
    } else {
        // Units tab: vehicles then infantry
        auto avail_units = player_house.available_units();
        for (UnitType ut : avail_units) {
            BuildIcon icon;
            icon.type_index = static_cast<int>(ut);
            icon.category = ProductionCategory::Unit;
            icon.name = unit_name(ut);
            icon.cost = registry.get_unit(ut).cost;
            icon.available = true;

            const auto* prod = player_house.current_production(
                ProductionCategory::Unit);
            if (prod && prod->type_index == icon.type_index) {
                icon.building = true;
                icon.progress = prod->progress;
                icon.ready = prod->ready;
            }

            current_icons_.push_back(icon);
        }

        auto avail_inf = player_house.available_infantry();
        for (InfantryType it : avail_inf) {
            BuildIcon icon;
            icon.type_index = static_cast<int>(it);
            icon.category = ProductionCategory::Infantry;
            icon.name = infantry_name(it);
            icon.cost = registry.get_infantry(it).cost;
            icon.available = true;

            const auto* prod = player_house.current_production(
                ProductionCategory::Infantry);
            if (prod && prod->type_index == icon.type_index) {
                icon.building = true;
                icon.progress = prod->progress;
                icon.ready = prod->ready;
            }

            current_icons_.push_back(icon);
        }
    }

    // Calculate icon positions
    int icon_w = (width_ - 8) / icons_per_row_;
    int icon_h = 40;
    int ox = icons_rect_.x + 2;
    int oy = icons_rect_.y + 2 - scroll_offset_;

    for (size_t i = 0; i < current_icons_.size(); i++) {
        int col = static_cast<int>(i) % icons_per_row_;
        int row = static_cast<int>(i) / icons_per_row_;
        current_icons_[i].bounds = Rect(
            ox + col * icon_w, oy + row * icon_h,
            icon_w - 2, icon_h - 2
        );
    }
}

// ---- Rendering ----

void Sidebar::render(Renderer& renderer, const House& player_house,
                      const TileMap* minimap_source) {
    // Background
    renderer.draw_rect_filled(sidebar_rect_, Color(30, 30, 30));
    renderer.draw_line(sidebar_rect_.x, 0, sidebar_rect_.x,
                       screen_height_, Color(80, 80, 80));

    render_credits(renderer, player_house);
    render_power_bar(renderer, player_house);
    render_minimap_frame(renderer, minimap_source);
    render_tab_buttons(renderer);
    render_build_icons(renderer, player_house);
    render_superweapons(renderer, player_house);
}

void Sidebar::render_credits(Renderer& renderer, const House& house) {
    renderer.draw_rect_filled(credits_rect_, Color(20, 20, 20));

    // Credits text
    Color credit_color = Color(0, 200, 0);
    renderer.draw_text("$" + std::to_string(house.credits()),
                       credits_rect_.x + 4, credits_rect_.y + 4,
                       credit_color, 10);
}

void Sidebar::render_power_bar(Renderer& renderer, const House& house) {
    // Power bar background
    renderer.draw_rect_filled(power_rect_, Color(20, 20, 20));
    renderer.draw_rect(power_rect_, Color(60, 60, 60));

    if (house.power_output() == 0 && house.power_drain() == 0) return;

    // Green portion (output) and red portion (drain)
    int total = std::max(house.power_output(), house.power_drain());
    if (total == 0) return;

    float output_pct = static_cast<float>(house.power_output()) / total;
    float drain_pct = static_cast<float>(house.power_drain()) / total;

    int bar_h = power_rect_.h - 4;
    int green_h = static_cast<int>(bar_h * output_pct);
    int red_h = static_cast<int>(bar_h * drain_pct);

    // Green bar (from bottom)
    renderer.draw_rect_filled(
        Rect(power_rect_.x + 2, power_rect_.y + 2 + bar_h - green_h,
             power_rect_.w - 4, green_h),
        Color(0, 180, 0));

    // Red line showing drain level
    int drain_y = power_rect_.y + 2 + bar_h - red_h;
    renderer.draw_line(power_rect_.x, drain_y,
                       power_rect_.x + power_rect_.w, drain_y,
                       Color(255, 0, 0));
}

void Sidebar::render_minimap_frame(Renderer& renderer, const TileMap* map) {
    renderer.draw_rect_filled(minimap_rect_, Color(0, 0, 0));
    if (map) {
        map->render_minimap(renderer, minimap_rect_);
    }
    renderer.draw_rect(minimap_rect_, Color(100, 100, 100));
}

void Sidebar::render_tab_buttons(Renderer& renderer) {
    int half_w = width_ / 2;

    // Structures tab
    Color tab1_color = (current_tab_ == SidebarTab::Structures) ?
                        Color(60, 60, 80) : Color(40, 40, 40);
    Rect tab1(tab_rect_.x, tab_rect_.y, half_w, tab_rect_.h);
    renderer.draw_rect_filled(tab1, tab1_color);
    renderer.draw_rect(tab1, Color(80, 80, 80));
    renderer.draw_text("BUILD", tab1.x + 8, tab1.y + 6,
                       Color::White(), 8);

    // Units tab
    Color tab2_color = (current_tab_ == SidebarTab::Units) ?
                        Color(60, 60, 80) : Color(40, 40, 40);
    Rect tab2(tab_rect_.x + half_w, tab_rect_.y, half_w, tab_rect_.h);
    renderer.draw_rect_filled(tab2, tab2_color);
    renderer.draw_rect(tab2, Color(80, 80, 80));
    renderer.draw_text("UNITS", tab2.x + 8, tab2.y + 6,
                       Color::White(), 8);
}

void Sidebar::render_build_icons(Renderer& renderer, const House& house) {
    // Clip to icons area
    for (const auto& icon : current_icons_) {
        // Skip if outside visible area
        if (icon.bounds.y + icon.bounds.h < icons_rect_.y) continue;
        if (icon.bounds.y > icons_rect_.y + icons_rect_.h) continue;

        // Background
        Color bg = icon.available ? Color(50, 50, 50) : Color(30, 30, 30);
        if (icon.ready) bg = Color(0, 80, 0);
        renderer.draw_rect_filled(icon.bounds, bg);

        // Border
        Color border = icon.available ? Color(100, 100, 100) : Color(50, 50, 50);
        if (icon.ready) border = Color(0, 200, 0);
        renderer.draw_rect(icon.bounds, border);

        // Name (abbreviated)
        std::string label = icon.name;
        if (label.length() > 8) label = label.substr(0, 8);
        renderer.draw_text(label, icon.bounds.x + 3, icon.bounds.y + 3,
                          Color::White(), 7);

        // Cost
        renderer.draw_text("$" + std::to_string(icon.cost),
                          icon.bounds.x + 3,
                          icon.bounds.y + icon.bounds.h - 12,
                          Color(0, 200, 0), 7);

        // Production progress bar
        if (icon.building && !icon.ready) {
            int bar_w = icon.bounds.w - 4;
            int fill_w = static_cast<int>(bar_w * icon.progress);
            int bar_y = icon.bounds.y + icon.bounds.h / 2;

            renderer.draw_rect_filled(
                Rect(icon.bounds.x + 2, bar_y, bar_w, 4),
                Color(40, 40, 40));
            renderer.draw_rect_filled(
                Rect(icon.bounds.x + 2, bar_y, fill_w, 4),
                Color(0, 200, 200));
        }

        if (icon.ready) {
            renderer.draw_text("READY", icon.bounds.x + 3,
                              icon.bounds.y + icon.bounds.h / 2 - 4,
                              Color(0, 255, 0), 8);
        }
    }
}

void Sidebar::render_superweapons(Renderer& renderer, const House& house) {
    renderer.draw_rect_filled(super_rect_, Color(20, 20, 20));

    // Ion Cannon (GDI) or Nuke (Nod)
    const auto& sw = (house.type() == HouseType::GDI) ?
                      house.ion_cannon() : house.ion_cannon(); // TODO: nuke

    if (sw.available) {
        int bar_w = width_ - 8;
        int fill_w = static_cast<int>(bar_w * sw.charge_progress);

        Color bar_color = sw.ready ? Color(0, 255, 0) : Color(0, 100, 200);
        renderer.draw_rect_filled(
            Rect(super_rect_.x + 4, super_rect_.y + 4, bar_w, 8),
            Color(40, 40, 40));
        renderer.draw_rect_filled(
            Rect(super_rect_.x + 4, super_rect_.y + 4, fill_w, 8),
            bar_color);

        const char* label = sw.ready ? "READY" : "CHARGING";
        renderer.draw_text(label, super_rect_.x + 4, super_rect_.y + 14,
                          bar_color, 7);
    }
}

bool Sidebar::handle_click(int mouse_x, int mouse_y, MouseButton button) {
    if (!is_in_sidebar(mouse_x, mouse_y)) return false;

    // Tab click
    if (tab_rect_.contains(mouse_x, mouse_y)) {
        int half_w = width_ / 2;
        if (mouse_x < tab_rect_.x + half_w) {
            current_tab_ = SidebarTab::Structures;
        } else {
            current_tab_ = SidebarTab::Units;
        }
        return true;
    }

    // Icon click
    for (const auto& icon : current_icons_) {
        if (icon.bounds.contains(mouse_x, mouse_y) && icon.available) {
            if (build_callback_) {
                build_callback_(icon.category, icon.type_index);
            }
            return true;
        }
    }

    return true; // Consume click even if nothing specific was hit
}

bool Sidebar::is_in_sidebar(int mouse_x, int /*mouse_y*/) const {
    return mouse_x >= sidebar_rect_.x;
}

// ============================================================
// Game Input Handler
// ============================================================

GameInputHandler::GameInputHandler() = default;

void GameInputHandler::init(int screen_width, int screen_height,
                             int sidebar_width) {
    screen_width_ = screen_width;
    screen_height_ = screen_height;
    game_area_width_ = screen_width - sidebar_width;
}

bool GameInputHandler::handle_event(const InputEvent& event,
                                     GameSession& session,
                                     Sidebar& sidebar,
                                     Renderer& renderer) {
    switch (event.type) {
        case InputEvent::Type::MouseMove:
            mouse_x_ = event.mouse_x;
            mouse_y_ = event.mouse_y;

            if (drag_selecting_) {
                drag_end_x_ = event.mouse_x;
                drag_end_y_ = event.mouse_y;
            }

            if (placing_building_) {
                // Update placement preview
                Point world = renderer.screen_to_world(event.mouse_x,
                                                        event.mouse_y);
                placement_cell_x_ = world.x / TILE_RENDER_SIZE;
                placement_cell_y_ = world.y / TILE_RENDER_SIZE;
            }
            return false;

        case InputEvent::Type::MouseDown:
            if (event.button == MouseButton::Left) {
                // Check sidebar first
                if (sidebar.is_in_sidebar(event.mouse_x, event.mouse_y)) {
                    sidebar.handle_click(event.mouse_x, event.mouse_y,
                                         event.button);
                    return true;
                }

                if (placing_building_) {
                    // Place building
                    handle_left_click(event.mouse_x, event.mouse_y,
                                      session, sidebar, renderer);
                    return true;
                }

                // Start drag selection
                drag_selecting_ = true;
                drag_start_x_ = event.mouse_x;
                drag_start_y_ = event.mouse_y;
                drag_end_x_ = event.mouse_x;
                drag_end_y_ = event.mouse_y;
            } else if (event.button == MouseButton::Right) {
                if (placing_building_) {
                    cancel_building_placement();
                } else {
                    handle_right_click(event.mouse_x, event.mouse_y,
                                       session, renderer);
                }
                return true;
            }
            return false;

        case InputEvent::Type::MouseUp:
            if (event.button == MouseButton::Left && drag_selecting_) {
                drag_selecting_ = false;
                drag_end_x_ = event.mouse_x;
                drag_end_y_ = event.mouse_y;

                // If drag was small, treat as click
                int dx = std::abs(drag_end_x_ - drag_start_x_);
                int dy = std::abs(drag_end_y_ - drag_start_y_);

                if (dx < 4 && dy < 4) {
                    handle_left_click(event.mouse_x, event.mouse_y,
                                      session, sidebar, renderer);
                } else {
                    // Box selection
                    Rect sel = get_selection_rect();
                    Point w1 = renderer.screen_to_world(sel.x, sel.y);
                    Point w2 = renderer.screen_to_world(sel.x + sel.w,
                                                         sel.y + sel.h);
                    Rect world_sel(w1.x, w1.y, w2.x - w1.x, w2.y - w1.y);

                    session.entities().select_units_in_rect(
                        world_sel, session.player_house().type());
                }
            }
            return false;

        case InputEvent::Type::KeyDown:
            handle_key(event.key, session);

            // Arrow key scrolling
            if (event.key == KeyCode::Left || event.key == KeyCode::A)
                scroll_left_ = true;
            if (event.key == KeyCode::Right || event.key == KeyCode::D)
                scroll_right_ = true;
            if (event.key == KeyCode::Up || event.key == KeyCode::W)
                scroll_up_ = true;
            if (event.key == KeyCode::Down || event.key == KeyCode::S)
                scroll_down_ = true;
            return false;

        case InputEvent::Type::KeyUp:
            if (event.key == KeyCode::Left || event.key == KeyCode::A)
                scroll_left_ = false;
            if (event.key == KeyCode::Right || event.key == KeyCode::D)
                scroll_right_ = false;
            if (event.key == KeyCode::Up || event.key == KeyCode::W)
                scroll_up_ = false;
            if (event.key == KeyCode::Down || event.key == KeyCode::S)
                scroll_down_ = false;
            return false;

        default:
            return false;
    }
}

void GameInputHandler::update(float dt, Renderer& renderer) {
    update_scroll(dt);
    renderer.set_camera(camera_x_, camera_y_);
}

void GameInputHandler::update_scroll(float dt) {
    float speed = scroll_speed_ * dt;

    // Edge scrolling
    bool edge_left = mouse_x_ < edge_scroll_margin_;
    bool edge_right = mouse_x_ > game_area_width_ - edge_scroll_margin_;
    bool edge_up = mouse_y_ < edge_scroll_margin_;
    bool edge_down = mouse_y_ > screen_height_ - edge_scroll_margin_;

    if (scroll_left_ || edge_left) camera_x_ -= static_cast<int>(speed);
    if (scroll_right_ || edge_right) camera_x_ += static_cast<int>(speed);
    if (scroll_up_ || edge_up) camera_y_ -= static_cast<int>(speed);
    if (scroll_down_ || edge_down) camera_y_ += static_cast<int>(speed);

    // Clamp to map bounds
    camera_x_ = std::max(0, camera_x_);
    camera_y_ = std::max(0, camera_y_);
}

Rect GameInputHandler::get_selection_rect() const {
    int x = std::min(drag_start_x_, drag_end_x_);
    int y = std::min(drag_start_y_, drag_end_y_);
    int w = std::abs(drag_end_x_ - drag_start_x_);
    int h = std::abs(drag_end_y_ - drag_start_y_);
    return Rect(x, y, w, h);
}

void GameInputHandler::handle_left_click(int mx, int my,
                                          GameSession& session,
                                          Sidebar& sidebar,
                                          Renderer& renderer) {
    if (placing_building_) {
        // Try to place building
        EntityID id = session.place_building(
            placing_type_, session.player_house().type(),
            placement_cell_x_, placement_cell_y_
        );
        if (id != INVALID_ENTITY) {
            session.player_house().place_produced(ProductionCategory::Structure);
            cancel_building_placement();
        }
        return;
    }

    // Single click: select unit at click position
    Point world = renderer.screen_to_world(mx, my);
    Rect click_area(world.x - 8, world.y - 8, 16, 16);

    session.entities().clear_selection();
    auto units = session.entities().get_units_in_area(click_area);
    for (EntityID id : units) {
        if (auto* u = session.entities().get_unit(id)) {
            if (u->owner == session.player_house().type()) {
                u->selected = true;
                break; // Select only one on click
            }
        }
        if (auto* i = session.entities().get_infantry(id)) {
            if (i->owner == session.player_house().type()) {
                i->selected = true;
                break;
            }
        }
    }
}

void GameInputHandler::handle_right_click(int mx, int my,
                                           GameSession& session,
                                           Renderer& renderer) {
    // Right click: command selected units
    auto selected = session.entities().get_selected();
    if (selected.empty()) return;

    Point world = renderer.screen_to_world(mx, my);

    // Check if clicking on an enemy -> attack
    Rect click_area(world.x - 8, world.y - 8, 16, 16);
    auto targets = session.entities().get_units_in_area(click_area);

    EntityID enemy_target = INVALID_ENTITY;
    for (EntityID tid : targets) {
        if (auto* u = session.entities().get_unit(tid)) {
            if (u->owner != session.player_house().type()) {
                enemy_target = tid;
                break;
            }
        }
    }

    if (enemy_target != INVALID_ENTITY) {
        session.command_attack(selected, enemy_target);
    } else {
        session.command_move(selected, world.x, world.y);
    }
}

void GameInputHandler::handle_key(KeyCode key, GameSession& session) {
    switch (key) {
        case KeyCode::S:
            // S key: stop selected units
            if (/* TODO: check if no modifier */ false) {
                auto selected = session.entities().get_selected();
                session.command_stop(selected);
            }
            break;

        case KeyCode::H:
            // H: harvest
            {
                auto selected = session.entities().get_selected();
                for (EntityID id : selected) {
                    session.command_harvest(id);
                }
            }
            break;

        default:
            break;
    }
}

void GameInputHandler::start_building_placement(BuildingType type) {
    placing_building_ = true;
    placing_type_ = type;
}

void GameInputHandler::cancel_building_placement() {
    placing_building_ = false;
    placing_type_ = BuildingType::None;
    placement_cell_x_ = -1;
    placement_cell_y_ = -1;
}

} // namespace CnC
