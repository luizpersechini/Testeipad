/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Entity Manager, Pathfinding, and Combat Implementation
 */

#include "unit.h"
#include "../graphics/tilemap.h"
#include <algorithm>
#include <queue>
#include <cmath>
#include <unordered_set>

namespace CnC {

// ============================================================
// Entity Manager
// ============================================================

EntityManager::EntityManager() {
    units_.reserve(MAX_UNITS);
    infantry_.reserve(MAX_INFANTRY);
    buildings_.reserve(MAX_BUILDINGS);
    projectiles_.reserve(MAX_BULLETS);
}

EntityManager::~EntityManager() = default;

EntityID EntityManager::create_unit(UnitType type, HouseType owner,
                                     float x, float y) {
    const auto& data = GameDataRegistry::instance().get_unit(type);

    Unit unit;
    unit.id = next_id_++;
    unit.type = type;
    unit.type_data = &data;
    unit.owner = owner;
    unit.x = x;
    unit.y = y;
    unit.hit_points = data.hit_points;
    unit.max_hit_points = data.hit_points;
    unit.move_speed = static_cast<float>(data.speed);
    unit.mission = MissionType::Guard;

    unit_index_[unit.id] = units_.size();
    units_.push_back(unit);

    return unit.id;
}

EntityID EntityManager::create_infantry(InfantryType type, HouseType owner,
                                         float x, float y) {
    const auto& data = GameDataRegistry::instance().get_infantry(type);

    Infantry inf;
    inf.id = next_id_++;
    inf.type = type;
    inf.type_data = &data;
    inf.owner = owner;
    inf.x = x;
    inf.y = y;
    inf.hit_points = data.hit_points;
    inf.max_hit_points = data.hit_points;
    inf.move_speed = static_cast<float>(data.speed);
    inf.mission = MissionType::Guard;

    infantry_index_[inf.id] = infantry_.size();
    infantry_.push_back(inf);

    return inf.id;
}

EntityID EntityManager::create_building(BuildingType type, HouseType owner,
                                         int cell_x, int cell_y) {
    const auto& data = GameDataRegistry::instance().get_building(type);

    Building bld;
    bld.id = next_id_++;
    bld.type = type;
    bld.type_data = &data;
    bld.owner = owner;
    bld.x = static_cast<float>(cell_x * CELL_PIXEL_WIDTH +
                                (data.width_cells * CELL_PIXEL_WIDTH) / 2);
    bld.y = static_cast<float>(cell_y * CELL_PIXEL_HEIGHT +
                                (data.height_cells * CELL_PIXEL_HEIGHT) / 2);
    bld.hit_points = data.hit_points;
    bld.max_hit_points = data.hit_points;
    bld.width_cells = data.width_cells;
    bld.height_cells = data.height_cells;
    bld.power_output = data.power_output;
    bld.mission = MissionType::Guard;

    building_index_[bld.id] = buildings_.size();
    buildings_.push_back(bld);

    return bld.id;
}

EntityID EntityManager::create_projectile(WeaponType weapon,
                                           float x, float y,
                                           float target_x, float target_y,
                                           int damage, EntityID source) {
    Projectile proj;
    proj.id = next_id_++;
    proj.weapon = weapon;
    proj.x = x;
    proj.y = y;
    proj.target_x = target_x;
    proj.target_y = target_y;
    proj.damage = damage;
    proj.source = source;

    // Speed varies by weapon type
    switch (weapon) {
        case WeaponType::Obelisk:
            proj.speed = 100.0f; // Instant
            break;
        case WeaponType::Rocket:
        case WeaponType::MammothTusk:
        case WeaponType::SSM:
            proj.speed = 8.0f;
            break;
        case WeaponType::Artillery:
            proj.speed = 6.0f;
            break;
        case WeaponType::Grenade:
            proj.speed = 5.0f;
            break;
        default:
            proj.speed = 15.0f; // Bullets
            break;
    }

    projectiles_.push_back(proj);
    return proj.id;
}

void EntityManager::destroy_unit(EntityID id) {
    auto it = unit_index_.find(id);
    if (it == unit_index_.end()) return;

    size_t idx = it->second;
    if (idx < units_.size() - 1) {
        // Swap with last element
        std::swap(units_[idx], units_.back());
        unit_index_[units_[idx].id] = idx;
    }
    units_.pop_back();
    unit_index_.erase(it);
}

void EntityManager::destroy_infantry(EntityID id) {
    auto it = infantry_index_.find(id);
    if (it == infantry_index_.end()) return;

    size_t idx = it->second;
    if (idx < infantry_.size() - 1) {
        std::swap(infantry_[idx], infantry_.back());
        infantry_index_[infantry_[idx].id] = idx;
    }
    infantry_.pop_back();
    infantry_index_.erase(it);
}

void EntityManager::destroy_building(EntityID id) {
    auto it = building_index_.find(id);
    if (it == building_index_.end()) return;

    size_t idx = it->second;
    if (idx < buildings_.size() - 1) {
        std::swap(buildings_[idx], buildings_.back());
        building_index_[buildings_[idx].id] = idx;
    }
    buildings_.pop_back();
    building_index_.erase(it);
}

Unit* EntityManager::get_unit(EntityID id) {
    auto it = unit_index_.find(id);
    if (it == unit_index_.end()) return nullptr;
    return &units_[it->second];
}

Infantry* EntityManager::get_infantry(EntityID id) {
    auto it = infantry_index_.find(id);
    if (it == infantry_index_.end()) return nullptr;
    return &infantry_[it->second];
}

Building* EntityManager::get_building(EntityID id) {
    auto it = building_index_.find(id);
    if (it == building_index_.end()) return nullptr;
    return &buildings_[it->second];
}

// ============================================================
// Queries
// ============================================================

std::vector<EntityID> EntityManager::get_units_in_area(const Rect& area) const {
    std::vector<EntityID> result;
    for (const auto& u : units_) {
        if (area.contains(static_cast<int>(u.x), static_cast<int>(u.y))) {
            result.push_back(u.id);
        }
    }
    for (const auto& i : infantry_) {
        if (area.contains(static_cast<int>(i.x), static_cast<int>(i.y))) {
            result.push_back(i.id);
        }
    }
    return result;
}

std::vector<EntityID> EntityManager::get_entities_at_cell(int cx, int cy) const {
    std::vector<EntityID> result;
    float x_min = cx * CELL_PIXEL_WIDTH;
    float x_max = x_min + CELL_PIXEL_WIDTH;
    float y_min = cy * CELL_PIXEL_HEIGHT;
    float y_max = y_min + CELL_PIXEL_HEIGHT;

    for (const auto& u : units_) {
        if (u.x >= x_min && u.x < x_max && u.y >= y_min && u.y < y_max)
            result.push_back(u.id);
    }
    for (const auto& i : infantry_) {
        if (i.x >= x_min && i.x < x_max && i.y >= y_min && i.y < y_max)
            result.push_back(i.id);
    }
    return result;
}

std::vector<EntityID> EntityManager::get_units_by_owner(HouseType owner) const {
    std::vector<EntityID> result;
    for (const auto& u : units_) {
        if (u.owner == owner) result.push_back(u.id);
    }
    for (const auto& i : infantry_) {
        if (i.owner == owner) result.push_back(i.id);
    }
    return result;
}

// ============================================================
// Selection
// ============================================================

void EntityManager::select_units_in_rect(const Rect& rect, HouseType player) {
    clear_selection();

    for (auto& u : units_) {
        if (u.owner == player &&
            rect.contains(static_cast<int>(u.x), static_cast<int>(u.y))) {
            u.selected = true;
        }
    }
    for (auto& i : infantry_) {
        if (i.owner == player &&
            rect.contains(static_cast<int>(i.x), static_cast<int>(i.y))) {
            i.selected = true;
        }
    }
}

void EntityManager::clear_selection() {
    for (auto& u : units_) u.selected = false;
    for (auto& i : infantry_) i.selected = false;
    for (auto& b : buildings_) b.selected = false;
}

std::vector<EntityID> EntityManager::get_selected() const {
    std::vector<EntityID> result;
    for (const auto& u : units_) {
        if (u.selected) result.push_back(u.id);
    }
    for (const auto& i : infantry_) {
        if (i.selected) result.push_back(i.id);
    }
    for (const auto& b : buildings_) {
        if (b.selected) result.push_back(b.id);
    }
    return result;
}

// ============================================================
// Update
// ============================================================

void EntityManager::update(float dt, TileMap& map) {
    // Update units
    for (auto& unit : units_) {
        if (!unit.is_alive()) continue;

        // Movement
        if (unit.is_moving && !unit.path.empty()) {
            // Move towards next waypoint
            Point target;
            if (unit.path_index < static_cast<int>(unit.path.size())) {
                target = unit.path[unit.path_index];
            } else {
                unit.is_moving = false;
                unit.mission = MissionType::Guard;
                continue;
            }

            float tx = target.x * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2.0f;
            float ty = target.y * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2.0f;

            float dx = tx - unit.x;
            float dy = ty - unit.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 2.0f) {
                // Reached waypoint
                unit.x = tx;
                unit.y = ty;
                unit.path_index++;

                if (unit.path_index >= static_cast<int>(unit.path.size())) {
                    unit.is_moving = false;
                    unit.mission = MissionType::Guard;
                }
            } else {
                // Move towards waypoint
                float speed = unit.move_speed * CELL_PIXEL_WIDTH * dt;
                unit.x += (dx / dist) * speed;
                unit.y += (dy / dist) * speed;

                // Update facing based on movement direction
                float angle = std::atan2(dy, dx);
                // Convert to 8-directional facing (0=N, clockwise)
                int facing = static_cast<int>(
                    std::round((angle + M_PI) / (M_PI / 4.0))) % 8;
                // Remap: atan2 gives E=0, we need N=0
                unit.facing = (facing + 2) % 8;
            }
        }

        // Combat cooldown
        if (unit.fire_cooldown > 0) {
            unit.fire_cooldown -= dt;
        }

        // Harvesting
        if (unit.type_data && unit.type_data->is_harvester &&
            unit.mission == MissionType::Harvest) {
            int cx = unit.cell_x();
            int cy = unit.cell_y();
            if (map.get_tiberium_value(cx, cy) > 0 &&
                unit.tiberium_load < Unit::MAX_TIBERIUM_LOAD) {
                int harvested = map.harvest_tiberium(cx, cy, 1);
                unit.tiberium_load += harvested;
            }
            if (unit.tiberium_load >= Unit::MAX_TIBERIUM_LOAD) {
                unit.mission = MissionType::Return;
            }
        }
    }

    // Update infantry
    for (auto& inf : infantry_) {
        if (!inf.is_alive()) continue;

        if (inf.is_moving && !inf.path.empty()) {
            Point target;
            if (inf.path_index < static_cast<int>(inf.path.size())) {
                target = inf.path[inf.path_index];
            } else {
                inf.is_moving = false;
                inf.mission = MissionType::Guard;
                continue;
            }

            float tx = target.x * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2.0f;
            float ty = target.y * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2.0f;

            float dx = tx - inf.x;
            float dy = ty - inf.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 2.0f) {
                inf.x = tx;
                inf.y = ty;
                inf.path_index++;
                if (inf.path_index >= static_cast<int>(inf.path.size())) {
                    inf.is_moving = false;
                    inf.mission = MissionType::Guard;
                }
            } else {
                float speed = inf.move_speed * CELL_PIXEL_WIDTH * dt * 0.7f;
                inf.x += (dx / dist) * speed;
                inf.y += (dy / dist) * speed;

                float angle = std::atan2(dy, dx);
                inf.facing = (static_cast<int>(
                    std::round((angle + M_PI) / (M_PI / 4.0))) + 2) % 8;
            }
        }

        if (inf.fire_cooldown > 0) {
            inf.fire_cooldown -= dt;
        }
    }

    // Update buildings
    for (auto& bld : buildings_) {
        if (!bld.is_alive()) continue;

        // Construction progress
        if (bld.under_construction) {
            bld.construction_progress += dt * 0.1f; // 10 seconds to build
            if (bld.construction_progress >= 1.0f) {
                bld.under_construction = false;
                bld.construction_progress = 1.0f;
            }
        }

        // Production progress
        if (bld.producing) {
            bld.production_progress += dt * 0.05f; // Variable build times
            if (bld.production_progress >= 1.0f) {
                bld.producing = false;
                bld.production_progress = 1.0f;
                // TODO: Spawn the produced unit
            }
        }

        // Selling
        if (bld.selling) {
            bld.sell_progress += dt * 0.2f;
            if (bld.sell_progress >= 1.0f) {
                bld.hit_points = 0; // Mark for removal
            }
        }

        // Defensive fire cooldown
        if (bld.fire_cooldown > 0) {
            bld.fire_cooldown -= dt;
        }
    }

    // Clean up dead entities
    units_.erase(
        std::remove_if(units_.begin(), units_.end(),
                        [](const Unit& u) { return !u.is_alive(); }),
        units_.end());

    infantry_.erase(
        std::remove_if(infantry_.begin(), infantry_.end(),
                        [](const Infantry& i) { return !i.is_alive(); }),
        infantry_.end());

    buildings_.erase(
        std::remove_if(buildings_.begin(), buildings_.end(),
                        [](const Building& b) { return !b.is_alive(); }),
        buildings_.end());

    // Rebuild index maps after cleanup
    unit_index_.clear();
    for (size_t i = 0; i < units_.size(); i++)
        unit_index_[units_[i].id] = i;

    infantry_index_.clear();
    for (size_t i = 0; i < infantry_.size(); i++)
        infantry_index_[infantry_[i].id] = i;

    building_index_.clear();
    for (size_t i = 0; i < buildings_.size(); i++)
        building_index_[buildings_[i].id] = i;

    // Update projectiles
    update_projectiles(dt);
}

void EntityManager::update_projectiles(float dt) {
    for (auto& proj : projectiles_) {
        if (!proj.active) continue;

        float dx = proj.target_x - proj.x;
        float dy = proj.target_y - proj.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 3.0f) {
            // Hit target area
            proj.active = false;
            // TODO: Apply splash damage to units in area
            continue;
        }

        float move = proj.speed * CELL_PIXEL_WIDTH * dt;
        proj.x += (dx / dist) * move;
        proj.y += (dy / dist) * move;
    }

    // Remove inactive projectiles
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
                        [](const Projectile& p) { return !p.active; }),
        projectiles_.end());
}

// ============================================================
// Rendering
// ============================================================

void EntityManager::render(Renderer& renderer, const Rect& viewport) {
    // Render buildings first (below units)
    for (const auto& bld : buildings_) {
        if (!bld.is_alive()) continue;

        int sx = static_cast<int>(bld.x);
        int sy = static_cast<int>(bld.y);
        int half_w = (bld.width_cells * TILE_RENDER_SIZE) / 2;
        int half_h = (bld.height_cells * TILE_RENDER_SIZE) / 2;

        Color color;
        if (bld.owner == HouseType::GDI) {
            color = Color::GDI();
        } else if (bld.owner == HouseType::Nod) {
            color = Color::Nod();
        } else {
            color = Color(128, 128, 128);
        }

        // Building body
        Rect body(sx - half_w, sy - half_h, half_w * 2, half_h * 2);
        renderer.draw_rect_filled(body, color);
        renderer.draw_rect(body, Color::Black());

        // Construction animation
        if (bld.under_construction) {
            int progress_h = static_cast<int>(half_h * 2 *
                                               bld.construction_progress);
            Rect prog(sx - half_w, sy + half_h - progress_h,
                      half_w * 2, progress_h);
            renderer.draw_rect_filled(prog, Color(200, 200, 200, 100));
        }
    }

    // Render infantry
    for (const auto& inf : infantry_) {
        if (!inf.is_alive()) continue;

        int sx = static_cast<int>(inf.x);
        int sy = static_cast<int>(inf.y);

        Color color;
        if (inf.owner == HouseType::GDI) {
            color = Color::GDI();
        } else if (inf.owner == HouseType::Nod) {
            color = Color::Nod();
        } else {
            color = Color(128, 128, 128);
        }

        // Small circle for infantry
        int r = 4;
        Rect body(sx - r, sy - r, r * 2, r * 2);
        renderer.draw_rect_filled(body, color);
        renderer.draw_rect(body, Color::Black());
    }

    // Render units (vehicles)
    for (const auto& unit : units_) {
        if (!unit.is_alive()) continue;

        int sx = static_cast<int>(unit.x);
        int sy = static_cast<int>(unit.y);

        Color color;
        if (unit.owner == HouseType::GDI) {
            color = Color::GDI();
        } else if (unit.owner == HouseType::Nod) {
            color = Color::Nod();
        } else {
            color = Color(128, 128, 128);
        }

        // Vehicle body
        int half = TILE_RENDER_SIZE / 3;
        Rect body(sx - half, sy - half, half * 2, half * 2);
        renderer.draw_rect_filled(body, color);
        renderer.draw_rect(body, Color::Black());

        // Turret/barrel direction indicator
        int bx = sx + FACING_DX[unit.facing] * half;
        int by = sy + FACING_DY[unit.facing] * half;
        renderer.draw_line(sx, sy, bx, by, Color::Black());
    }

    // Render projectiles
    for (const auto& proj : projectiles_) {
        if (!proj.active) continue;

        int sx = static_cast<int>(proj.x);
        int sy = static_cast<int>(proj.y);

        Color color;
        switch (proj.weapon) {
            case WeaponType::Rocket:
            case WeaponType::MammothTusk:
            case WeaponType::SSM:
                color = Color(255, 200, 0); // Yellow for missiles
                break;
            case WeaponType::Obelisk:
                color = Color(255, 0, 0);   // Red for laser
                break;
            case WeaponType::Flamethrower:
            case WeaponType::Napalm:
                color = Color(255, 100, 0); // Orange for fire
                break;
            default:
                color = Color(255, 255, 200); // White for bullets
                break;
        }

        Rect bullet(sx - 2, sy - 2, 4, 4);
        renderer.draw_rect_filled(bullet, color);
    }
}

void EntityManager::render_health_bars(Renderer& renderer,
                                        const Rect& /*viewport*/) {
    auto draw_health_bar = [&](float x, float y, float ratio,
                                int width, bool is_selected) {
        if (ratio >= 1.0f && !is_selected) return;

        int bar_w = width;
        int bar_h = 3;
        int bx = static_cast<int>(x) - bar_w / 2;
        int by = static_cast<int>(y) - TILE_RENDER_SIZE / 2 - 6;

        // Background
        renderer.draw_rect_filled(Rect(bx, by, bar_w, bar_h),
                                  Color(0, 0, 0, 150));

        // Health bar
        Color bar_color;
        if (ratio > 0.5f) bar_color = Color(0, 200, 0);
        else if (ratio > 0.25f) bar_color = Color(255, 200, 0);
        else bar_color = Color(255, 0, 0);

        int fill_w = static_cast<int>(bar_w * ratio);
        renderer.draw_rect_filled(Rect(bx, by, fill_w, bar_h), bar_color);
    };

    for (const auto& u : units_) {
        if (!u.is_alive()) continue;
        draw_health_bar(u.x, u.y, u.health_ratio(),
                        TILE_RENDER_SIZE * 2 / 3, u.selected);
    }

    for (const auto& i : infantry_) {
        if (!i.is_alive()) continue;
        draw_health_bar(i.x, i.y, i.health_ratio(), 12, i.selected);
    }

    for (const auto& b : buildings_) {
        if (!b.is_alive()) continue;
        draw_health_bar(b.x, b.y, b.health_ratio(),
                        b.width_cells * TILE_RENDER_SIZE * 2 / 3,
                        b.selected);
    }
}

void EntityManager::render_selection_boxes(Renderer& renderer,
                                            const Rect& /*viewport*/) {
    Color sel_color(0, 255, 0, 200);

    for (const auto& u : units_) {
        if (!u.selected) continue;
        int half = TILE_RENDER_SIZE / 3 + 2;
        Rect sel(static_cast<int>(u.x) - half,
                 static_cast<int>(u.y) - half,
                 half * 2, half * 2);
        renderer.draw_rect(sel, sel_color);
    }

    for (const auto& i : infantry_) {
        if (!i.selected) continue;
        int half = 6;
        Rect sel(static_cast<int>(i.x) - half,
                 static_cast<int>(i.y) - half,
                 half * 2, half * 2);
        renderer.draw_rect(sel, sel_color);
    }

    for (const auto& b : buildings_) {
        if (!b.selected) continue;
        int half_w = (b.width_cells * TILE_RENDER_SIZE) / 2 + 2;
        int half_h = (b.height_cells * TILE_RENDER_SIZE) / 2 + 2;
        Rect sel(static_cast<int>(b.x) - half_w,
                 static_cast<int>(b.y) - half_h,
                 half_w * 2, half_h * 2);
        renderer.draw_rect(sel, sel_color);
    }
}

// ============================================================
// A* Pathfinding
// ============================================================

std::vector<Point> PathFinder::find_path(
    const TileMap& map,
    int start_x, int start_y,
    int end_x, int end_y,
    SpeedType speed_type) {

    if (!map.is_valid(start_x, start_y) || !map.is_valid(end_x, end_y))
        return {};

    if (!map.cell(end_x, end_y).passable(speed_type))
        return {};

    struct NodeEntry {
        int x, y;
        float g, h;
        float f() const { return g + h; }
        int parent_idx;
    };

    auto heuristic = [](int x1, int y1, int x2, int y2) -> float {
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        // Octile distance heuristic
        return static_cast<float>(std::max(dx, dy)) +
               0.41f * static_cast<float>(std::min(dx, dy));
    };

    // Encode cell coordinates as a single int for the visited set
    auto encode = [&](int x, int y) { return y * map.width() + x; };

    struct CompareNode {
        bool operator()(const std::pair<float, int>& a,
                        const std::pair<float, int>& b) {
            return a.first > b.first; // Min-heap
        }
    };

    std::vector<NodeEntry> nodes;
    nodes.reserve(256);

    std::unordered_map<int, int> visited; // encoded_pos -> node_index
    std::priority_queue<std::pair<float, int>,
                        std::vector<std::pair<float, int>>,
                        CompareNode> open;

    // Start node
    NodeEntry start_node{start_x, start_y, 0.0f,
                          heuristic(start_x, start_y, end_x, end_y), -1};
    nodes.push_back(start_node);
    open.push({start_node.f(), 0});
    visited[encode(start_x, start_y)] = 0;

    // 8-directional neighbors
    const int dx[] = { 0,  1,  1,  1,  0, -1, -1, -1};
    const int dy[] = {-1, -1,  0,  1,  1,  1,  0, -1};
    const float dcost[] = {1.0f, 1.41f, 1.0f, 1.41f,
                            1.0f, 1.41f, 1.0f, 1.41f};

    int max_iterations = 2000; // Prevent infinite loops

    while (!open.empty() && max_iterations > 0) {
        max_iterations--;

        auto [_, current_idx] = open.top();
        open.pop();

        const NodeEntry& current = nodes[current_idx];

        // Reached destination?
        if (current.x == end_x && current.y == end_y) {
            // Reconstruct path
            std::vector<Point> path;
            int idx = current_idx;
            while (idx >= 0) {
                path.push_back(Point(nodes[idx].x, nodes[idx].y));
                idx = nodes[idx].parent_idx;
            }
            std::reverse(path.begin(), path.end());
            // Remove start position from path
            if (!path.empty()) path.erase(path.begin());
            return path;
        }

        // Explore neighbors
        for (int dir = 0; dir < 8; dir++) {
            int nx = current.x + dx[dir];
            int ny = current.y + dy[dir];

            if (!map.is_valid(nx, ny)) continue;

            const MapCell& ncell = map.cell(nx, ny);
            if (!ncell.passable(speed_type)) continue;

            float move_cost = dcost[dir] * ncell.get_movement_cost(speed_type);
            float new_g = current.g + move_cost;

            int encoded = encode(nx, ny);
            auto vit = visited.find(encoded);

            if (vit != visited.end()) {
                // Already visited - check if this path is better
                if (new_g >= nodes[vit->second].g) continue;
                // Update existing node
                nodes[vit->second].g = new_g;
                nodes[vit->second].parent_idx = current_idx;
                open.push({new_g + nodes[vit->second].h, vit->second});
            } else {
                // New node
                float h = heuristic(nx, ny, end_x, end_y);
                NodeEntry new_node{nx, ny, new_g, h, current_idx};
                int new_idx = static_cast<int>(nodes.size());
                nodes.push_back(new_node);
                visited[encoded] = new_idx;
                open.push({new_g + h, new_idx});
            }
        }
    }

    return {}; // No path found
}

// ============================================================
// Combat System
// ============================================================

int CombatSystem::calculate_damage(WeaponType weapon, ArmorType armor,
                                    int base_damage) {
    int wi = static_cast<int>(weapon);
    int ai = static_cast<int>(armor);

    if (wi < 0 || wi >= 16 || ai < 0 || ai >= 5)
        return base_damage;

    float multiplier = DAMAGE_TABLE[wi][ai];
    int damage = static_cast<int>(base_damage * multiplier);

    // Randomize +/- 15%
    int variance = damage * 15 / 100;
    if (variance > 0) {
        damage += (std::rand() % (variance * 2 + 1)) - variance;
    }

    return std::max(1, damage);
}

bool CombatSystem::in_range(float src_x, float src_y,
                             float tgt_x, float tgt_y,
                             int range_cells) {
    float dx = tgt_x - src_x;
    float dy = tgt_y - src_y;
    float dist_sq = dx * dx + dy * dy;
    float range_pixels = range_cells * CELL_PIXEL_WIDTH;
    return dist_sq <= range_pixels * range_pixels;
}

void CombatSystem::fire_weapon(EntityManager& entities,
                                EntityID source, EntityID target,
                                const WeaponData& weapon) {
    // Get source position
    float sx = 0, sy = 0, tx = 0, ty = 0;

    if (auto* u = entities.get_unit(source)) {
        sx = u->x; sy = u->y;
    } else if (auto* i = entities.get_infantry(source)) {
        sx = i->x; sy = i->y;
    } else if (auto* b = entities.get_building(source)) {
        sx = b->x; sy = b->y;
    } else {
        return;
    }

    // Get target position
    if (auto* u = entities.get_unit(target)) {
        tx = u->x; ty = u->y;
    } else if (auto* i = entities.get_infantry(target)) {
        tx = i->x; ty = i->y;
    } else if (auto* b = entities.get_building(target)) {
        tx = b->x; ty = b->y;
    } else {
        return;
    }

    entities.create_projectile(weapon.type, sx, sy, tx, ty,
                               weapon.damage, source);
}

void CombatSystem::apply_damage(GameEntity& entity, int damage) {
    entity.hit_points -= damage;
    if (entity.hit_points < 0) entity.hit_points = 0;
}

} // namespace CnC
