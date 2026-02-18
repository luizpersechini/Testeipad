/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Main Menu Implementation
 */

#include "menu.h"
#include <cmath>
#include <algorithm>

namespace CnC {

MenuSystem::MenuSystem() = default;

void MenuSystem::init(int screen_width, int screen_height) {
    screen_w_ = screen_width;
    screen_h_ = screen_height;
    reset();
}

void MenuSystem::reset() {
    current_screen_ = MenuScreen::Main;
    start_game_ = false;
    quit_ = false;
    selected_index_ = 0;
    title_pulse_ = 0.0f;
    build_main_menu();
}

// ============================================================
// Menu Construction
// ============================================================

void MenuSystem::build_main_menu() {
    items_.clear();
    int cx = screen_w_ / 2;
    int base_y = screen_h_ / 2 + 20;
    int item_h = 28;

    MenuItem campaign;
    campaign.label = "CAMPAIGN";
    campaign.description = "Play the single-player campaign";
    campaign.id = 0;
    campaign.bounds = Rect(cx - 80, base_y, 160, item_h);
    items_.push_back(campaign);

    MenuItem skirmish;
    skirmish.label = "SKIRMISH";
    skirmish.description = "Battle against the AI";
    skirmish.id = 1;
    skirmish.bounds = Rect(cx - 80, base_y + item_h + 4, 160, item_h);
    items_.push_back(skirmish);

    MenuItem options;
    options.label = "OPTIONS";
    options.description = "Game settings";
    options.id = 2;
    options.bounds = Rect(cx - 80, base_y + (item_h + 4) * 2, 160, item_h);
    items_.push_back(options);

    MenuItem quit;
    quit.label = "QUIT";
    quit.description = "Exit game";
    quit.id = 3;
    quit.bounds = Rect(cx - 80, base_y + (item_h + 4) * 3, 160, item_h);
    items_.push_back(quit);

    selected_index_ = 0;
}

void MenuSystem::build_faction_select() {
    items_.clear();
    int cx = screen_w_ / 2;
    int base_y = screen_h_ / 2 + 10;
    int item_h = 50;
    int spacing = 10;

    MenuItem gdi;
    gdi.label = "GDI";
    gdi.description = "Global Defense Initiative - Heavy armor, Ion Cannon";
    gdi.id = 0;
    gdi.bounds = Rect(cx - 140, base_y, 130, item_h);
    items_.push_back(gdi);

    MenuItem nod;
    nod.label = "NOD";
    nod.description = "Brotherhood of Nod - Fast units, Stealth, Obelisk";
    nod.id = 1;
    nod.bounds = Rect(cx + 10, base_y, 130, item_h);
    items_.push_back(nod);

    selected_index_ = 0;
}

void MenuSystem::build_skirmish_setup() {
    items_.clear();
    int cx = screen_w_ / 2;
    int base_y = screen_h_ / 2 - 20;
    int item_h = 24;
    int spacing = 6;

    // Map size selector
    MenuItem map_size;
    map_size.label = std::string("MAP: ") + SkirmishSettings::map_size_name(skirmish_.map_size);
    map_size.description = "Left/Right to change";
    map_size.id = 0;
    map_size.bounds = Rect(cx - 100, base_y, 200, item_h);
    items_.push_back(map_size);

    // Difficulty selector
    MenuItem difficulty;
    difficulty.label = std::string("AI: ") + SkirmishSettings::difficulty_name(skirmish_.ai_difficulty);
    difficulty.description = "Left/Right to change";
    difficulty.id = 1;
    difficulty.bounds = Rect(cx - 100, base_y + item_h + spacing, 200, item_h);
    items_.push_back(difficulty);

    // Credits selector
    MenuItem credits;
    credits.label = "CREDITS: $" + std::to_string(skirmish_.starting_credits);
    credits.description = "Left/Right to change";
    credits.id = 2;
    credits.bounds = Rect(cx - 100, base_y + (item_h + spacing) * 2, 200, item_h);
    items_.push_back(credits);

    // Fog of war toggle
    MenuItem fog;
    fog.label = std::string("FOG OF WAR: ") + (skirmish_.fog_of_war ? "ON" : "OFF");
    fog.id = 3;
    fog.bounds = Rect(cx - 100, base_y + (item_h + spacing) * 3, 200, item_h);
    items_.push_back(fog);

    // Start button
    MenuItem start;
    start.label = "START BATTLE";
    start.description = "";
    start.id = 10;
    start.bounds = Rect(cx - 80, base_y + (item_h + spacing) * 4 + 10, 160, 30);
    items_.push_back(start);

    selected_index_ = 4; // Start button selected
}

void MenuSystem::build_campaign_select() {
    items_.clear();
    int cx = screen_w_ / 2;
    int base_y = screen_h_ / 2 + 10;
    int item_h = 24;

    MenuItem m1;
    m1.label = (campaign_faction_ == HouseType::GDI) ?
               "1: The Beachhead" : "1: Silencing Dissent";
    m1.id = 1;
    m1.bounds = Rect(cx - 100, base_y, 200, item_h);
    items_.push_back(m1);

    // Additional missions would be added here as they're implemented
    MenuItem back;
    back.label = "BACK";
    back.id = 0;
    back.bounds = Rect(cx - 100, base_y + item_h + 8, 200, item_h);
    items_.push_back(back);

    selected_index_ = 0;
}

// ============================================================
// Input
// ============================================================

void MenuSystem::handle_event(const InputEvent& event) {
    if (event.type == InputEvent::Type::KeyDown) {
        switch (event.key) {
            case KeyCode::Up:
                select_item(selected_index_ - 1);
                break;
            case KeyCode::Down:
                select_item(selected_index_ + 1);
                break;
            case KeyCode::Enter:
            case KeyCode::Space:
                confirm_selection();
                break;
            case KeyCode::Escape:
                go_back();
                break;
            case KeyCode::Left:
                // Adjust values in skirmish setup
                if (current_screen_ == MenuScreen::SkirmishSetup) {
                    if (selected_index_ == 0)
                        skirmish_.map_size = std::max(0, skirmish_.map_size - 1);
                    else if (selected_index_ == 1)
                        skirmish_.ai_difficulty = std::max(0, skirmish_.ai_difficulty - 1);
                    else if (selected_index_ == 2)
                        skirmish_.starting_credits = std::max(2000, skirmish_.starting_credits - 2000);
                    else if (selected_index_ == 3)
                        skirmish_.fog_of_war = !skirmish_.fog_of_war;
                    build_skirmish_setup();
                    select_item(selected_index_); // Preserve selection
                }
                break;
            case KeyCode::Right:
                if (current_screen_ == MenuScreen::SkirmishSetup) {
                    if (selected_index_ == 0)
                        skirmish_.map_size = std::min(2, skirmish_.map_size + 1);
                    else if (selected_index_ == 1)
                        skirmish_.ai_difficulty = std::min(2, skirmish_.ai_difficulty + 1);
                    else if (selected_index_ == 2)
                        skirmish_.starting_credits = std::min(50000, skirmish_.starting_credits + 2000);
                    else if (selected_index_ == 3)
                        skirmish_.fog_of_war = !skirmish_.fog_of_war;
                    build_skirmish_setup();
                    select_item(selected_index_);
                }
                break;
            default:
                break;
        }
    }

    // Mouse support
    if (event.type == InputEvent::Type::MouseMove) {
        for (int i = 0; i < static_cast<int>(items_.size()); i++) {
            if (items_[i].bounds.contains(event.mouse_x, event.mouse_y)) {
                selected_index_ = i;
                break;
            }
        }
    }

    if (event.type == InputEvent::Type::MouseDown &&
        event.button == MouseButton::Left) {
        for (int i = 0; i < static_cast<int>(items_.size()); i++) {
            if (items_[i].bounds.contains(event.mouse_x, event.mouse_y)) {
                selected_index_ = i;
                confirm_selection();
                break;
            }
        }
    }
}

void MenuSystem::select_item(int index) {
    if (items_.empty()) return;
    selected_index_ = std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
}

void MenuSystem::confirm_selection() {
    if (items_.empty()) return;

    const auto& item = items_[selected_index_];

    switch (current_screen_) {
        case MenuScreen::Main:
            switch (item.id) {
                case 0: // Campaign
                    current_screen_ = MenuScreen::FactionSelect;
                    campaign_mode_ = true;
                    build_faction_select();
                    break;
                case 1: // Skirmish
                    current_screen_ = MenuScreen::FactionSelect;
                    campaign_mode_ = false;
                    build_faction_select();
                    break;
                case 2: // Options
                    // TODO: options screen
                    break;
                case 3: // Quit
                    quit_ = true;
                    break;
            }
            break;

        case MenuScreen::FactionSelect:
            if (item.id == 0) {
                skirmish_.player_faction = HouseType::GDI;
                campaign_faction_ = HouseType::GDI;
            } else {
                skirmish_.player_faction = HouseType::Nod;
                campaign_faction_ = HouseType::Nod;
            }

            if (campaign_mode_) {
                current_screen_ = MenuScreen::CampaignMission;
                build_campaign_select();
            } else {
                current_screen_ = MenuScreen::SkirmishSetup;
                build_skirmish_setup();
            }
            break;

        case MenuScreen::SkirmishSetup:
            if (item.id == 10) { // Start button
                start_game_ = true;
            }
            break;

        case MenuScreen::CampaignMission:
            if (item.id == 0) { // Back
                go_back();
            } else {
                campaign_mission_ = item.id;
                campaign_mode_ = true;
                start_game_ = true;
            }
            break;

        default:
            break;
    }
}

void MenuSystem::go_back() {
    switch (current_screen_) {
        case MenuScreen::Main:
            quit_ = true;
            break;
        case MenuScreen::FactionSelect:
            current_screen_ = MenuScreen::Main;
            build_main_menu();
            break;
        case MenuScreen::SkirmishSetup:
        case MenuScreen::CampaignMission:
            current_screen_ = MenuScreen::FactionSelect;
            build_faction_select();
            break;
        default:
            current_screen_ = MenuScreen::Main;
            build_main_menu();
            break;
    }
}

// ============================================================
// Update
// ============================================================

void MenuSystem::update(float dt) {
    title_pulse_ += dt * 2.0f;
    if (title_pulse_ > 6.28f) title_pulse_ -= 6.28f;
}

// ============================================================
// Rendering
// ============================================================

void MenuSystem::render(Renderer& renderer) {
    // Dark background
    renderer.draw_rect_filled(Rect(0, 0, screen_w_, screen_h_),
                              Color(10, 10, 15));

    // Decorative lines
    Color line_color(30, 30, 40);
    for (int y = 0; y < screen_h_; y += 20) {
        renderer.draw_line(0, y, screen_w_, y, line_color);
    }

    render_title(renderer);

    switch (current_screen_) {
        case MenuScreen::FactionSelect:
            render_faction_select(renderer);
            break;
        case MenuScreen::SkirmishSetup:
            render_skirmish_setup(renderer);
            break;
        default:
            render_menu_items(renderer);
            break;
    }

    // Footer
    renderer.draw_text("Based on C&C: Tiberian Dawn by Westwood Studios",
                       screen_w_ / 2 - 160, screen_h_ - 20,
                       Color(60, 60, 60), 7);
    renderer.draw_text("Original source released under GPL v3 by Electronic Arts",
                       screen_w_ / 2 - 175, screen_h_ - 10,
                       Color(60, 60, 60), 7);
}

void MenuSystem::render_title(Renderer& renderer) {
    int cx = screen_w_ / 2;

    // Title with subtle pulse
    float pulse = 0.85f + 0.15f * std::sin(title_pulse_);
    uint8_t glow = static_cast<uint8_t>(255 * pulse);

    renderer.draw_text("COMMAND & CONQUER", cx - 100, 40,
                       Color(glow, static_cast<uint8_t>(glow * 0.85f), 0), 20);
    renderer.draw_text("TIBERIAN DAWN", cx - 75, 70,
                       Color::GDI(), 16);

    // Screen-specific subtitle
    const char* subtitle = nullptr;
    switch (current_screen_) {
        case MenuScreen::Main:
            subtitle = "MODERN PORT";
            break;
        case MenuScreen::FactionSelect:
            subtitle = "SELECT YOUR FACTION";
            break;
        case MenuScreen::SkirmishSetup:
            subtitle = "SKIRMISH SETUP";
            break;
        case MenuScreen::CampaignMission:
            subtitle = campaign_faction_ == HouseType::GDI ?
                       "GDI CAMPAIGN" : "NOD CAMPAIGN";
            break;
        default:
            break;
    }

    if (subtitle) {
        renderer.draw_text(subtitle, cx - 60, 95, Color::White(), 10);
    }

    // Faction color bar beneath title
    Color bar_color = Color(80, 80, 80);
    renderer.draw_rect_filled(Rect(cx - 120, 112, 240, 2), bar_color);
}

void MenuSystem::render_menu_items(Renderer& renderer) {
    for (int i = 0; i < static_cast<int>(items_.size()); i++) {
        const auto& item = items_[i];
        bool is_sel = (i == selected_index_);

        // Background
        Color bg = is_sel ? Color(40, 40, 60) : Color(20, 20, 25);
        renderer.draw_rect_filled(item.bounds, bg);

        // Selection indicator
        if (is_sel) {
            renderer.draw_rect(item.bounds, Color::GDI());
            // Left arrow
            renderer.draw_text(">", item.bounds.x - 12,
                              item.bounds.y + item.bounds.h / 2 - 5,
                              Color::GDI(), 10);
        } else {
            renderer.draw_rect(item.bounds, Color(50, 50, 50));
        }

        // Label
        Color text_color = is_sel ? Color::White() : Color(160, 160, 160);
        if (!item.enabled) text_color = Color(80, 80, 80);

        int text_x = item.bounds.x + item.bounds.w / 2 -
                     static_cast<int>(item.label.length()) * 3;
        renderer.draw_text(item.label, text_x,
                          item.bounds.y + item.bounds.h / 2 - 5,
                          text_color, 10);
    }

    // Description for selected item
    if (selected_index_ >= 0 &&
        selected_index_ < static_cast<int>(items_.size())) {
        const auto& desc = items_[selected_index_].description;
        if (!desc.empty()) {
            int desc_x = screen_w_ / 2 - static_cast<int>(desc.length()) * 2;
            renderer.draw_text(desc, desc_x, screen_h_ - 50,
                              Color(120, 120, 120), 8);
        }
    }
}

void MenuSystem::render_faction_select(Renderer& renderer) {
    for (int i = 0; i < static_cast<int>(items_.size()); i++) {
        const auto& item = items_[i];
        bool is_sel = (i == selected_index_);

        // Faction box
        Color faction_color = (item.id == 0) ? Color::GDI() : Color::Nod();
        Color bg = is_sel ? Color(40, 40, 50) : Color(20, 20, 25);

        renderer.draw_rect_filled(item.bounds, bg);

        // Faction emblem area
        Rect emblem(item.bounds.x + 10, item.bounds.y + 5,
                    item.bounds.w - 20, 20);
        renderer.draw_rect_filled(emblem, faction_color);

        // Faction name in emblem
        renderer.draw_text(item.label,
                          emblem.x + emblem.w / 2 - static_cast<int>(item.label.length()) * 4,
                          emblem.y + 4,
                          Color::Black(), 12);

        // Selection border
        if (is_sel) {
            renderer.draw_rect(item.bounds, faction_color);
            // Arrow
            renderer.draw_text("^", item.bounds.x + item.bounds.w / 2 - 3,
                              item.bounds.y + item.bounds.h + 3,
                              faction_color, 10);
        } else {
            renderer.draw_rect(item.bounds, Color(60, 60, 60));
        }

        // Description
        if (is_sel && !item.description.empty()) {
            int dx = item.bounds.x + item.bounds.w / 2 -
                     static_cast<int>(item.description.length()) * 2;
            renderer.draw_text(item.description, dx,
                              item.bounds.y + item.bounds.h + 18,
                              Color(140, 140, 140), 7);
        }
    }

    renderer.draw_text("Press ENTER to select, ESC to go back",
                       screen_w_ / 2 - 120, screen_h_ - 50,
                       Color(80, 80, 80), 8);
}

void MenuSystem::render_skirmish_setup(Renderer& renderer) {
    for (int i = 0; i < static_cast<int>(items_.size()); i++) {
        const auto& item = items_[i];
        bool is_sel = (i == selected_index_);

        // Different rendering for Start button vs settings
        if (item.id == 10) {
            // Start button
            Color bg = is_sel ? Color(0, 80, 0) : Color(0, 50, 0);
            renderer.draw_rect_filled(item.bounds, bg);
            Color border = is_sel ? Color(0, 200, 0) : Color(0, 100, 0);
            renderer.draw_rect(item.bounds, border);

            int tx = item.bounds.x + item.bounds.w / 2 -
                     static_cast<int>(item.label.length()) * 4;
            renderer.draw_text(item.label, tx,
                              item.bounds.y + item.bounds.h / 2 - 5,
                              Color::White(), 10);
        } else {
            // Setting row
            Color bg = is_sel ? Color(30, 30, 45) : Color(20, 20, 25);
            renderer.draw_rect_filled(item.bounds, bg);

            if (is_sel) {
                renderer.draw_rect(item.bounds, Color::GDI());
                // Left/right arrows
                renderer.draw_text("<", item.bounds.x + 4,
                                  item.bounds.y + 4, Color::GDI(), 10);
                renderer.draw_text(">", item.bounds.x + item.bounds.w - 12,
                                  item.bounds.y + 4, Color::GDI(), 10);
            }

            // Label centered
            Color lbl_color = is_sel ? Color::White() : Color(160, 160, 160);
            int tx = item.bounds.x + item.bounds.w / 2 -
                     static_cast<int>(item.label.length()) * 3;
            renderer.draw_text(item.label, tx, item.bounds.y + 5,
                              lbl_color, 9);
        }
    }

    // Show faction
    const char* faction = (skirmish_.player_faction == HouseType::GDI) ?
                          "GDI" : "NOD";
    Color fc = (skirmish_.player_faction == HouseType::GDI) ?
               Color::GDI() : Color::Nod();
    renderer.draw_text(std::string("PLAYING AS: ") + faction,
                       screen_w_ / 2 - 50, screen_h_ / 2 - 50, fc, 9);
}

} // namespace CnC
