/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Tile Map Implementation
 */

#include "tilemap.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace CnC {

MapCell TileMap::null_cell_;

TileMap::TileMap() = default;
TileMap::~TileMap() = default;

bool TileMap::init(Renderer& renderer, const std::string& /*tileset_path*/) {
    renderer_ = &renderer;
    return true;
}

void TileMap::shutdown() {
    cells_.clear();
    width_ = 0;
    height_ = 0;
}

void TileMap::create_empty(int width, int height) {
    width_ = width;
    height_ = height;
    cells_.resize(width * height);

    for (auto& cell : cells_) {
        cell = MapCell();
        cell.terrain = TerrainCellType::Clear;
        cell.tile_id = 0;
    }
}

bool TileMap::is_valid(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

MapCell& TileMap::cell(int x, int y) {
    if (!is_valid(x, y)) return null_cell_;
    return cells_[y * width_ + x];
}

const MapCell& TileMap::cell(int x, int y) const {
    if (!is_valid(x, y)) return null_cell_;
    return cells_[y * width_ + x];
}

MapCell& TileMap::cell_at(int index) {
    if (index < 0 || index >= static_cast<int>(cells_.size()))
        return null_cell_;
    return cells_[index];
}

Point TileMap::cell_to_pixel(int cell_x, int cell_y) const {
    return Point(cell_x * tile_render_size_, cell_y * tile_render_size_);
}

Point TileMap::pixel_to_cell(int pixel_x, int pixel_y) const {
    return Point(pixel_x / tile_render_size_, pixel_y / tile_render_size_);
}

// ============================================================
// Map Generation
// ============================================================

void TileMap::generate_test_map(int width, int height) {
    create_empty(width, height);

    std::srand(42); // Fixed seed for reproducibility

    // Fill with clear terrain
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto& c = cell(x, y);
            c.terrain = TerrainCellType::Clear;
            c.tile_id = 0;
        }
    }

    // Add some sand patches
    for (int i = 0; i < 8; i++) {
        int cx = std::rand() % width;
        int cy = std::rand() % height;
        int radius = 2 + std::rand() % 4;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy <= radius * radius) {
                    int nx = cx + dx, ny = cy + dy;
                    if (is_valid(nx, ny)) {
                        cell(nx, ny).terrain = TerrainCellType::Sand;
                        cell(nx, ny).tile_id = 1;
                    }
                }
            }
        }
    }

    // Add rock formations (impassable)
    for (int i = 0; i < 5; i++) {
        int cx = std::rand() % width;
        int cy = std::rand() % height;
        int radius = 1 + std::rand() % 2;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (std::abs(dx) + std::abs(dy) <= radius) {
                    int nx = cx + dx, ny = cy + dy;
                    if (is_valid(nx, ny)) {
                        cell(nx, ny).terrain = TerrainCellType::Rock;
                        cell(nx, ny).tile_id = 2;
                    }
                }
            }
        }
    }

    // Add a river
    int river_x = width / 3;
    for (int y = 0; y < height; y++) {
        int rx = river_x + static_cast<int>(std::sin(y * 0.3) * 2);
        for (int dx = 0; dx < 2; dx++) {
            if (is_valid(rx + dx, y)) {
                cell(rx + dx, y).terrain = TerrainCellType::Water;
                cell(rx + dx, y).tile_id = 3;
            }
        }
    }

    // Add a road
    int road_y = height / 2;
    for (int x = 0; x < width; x++) {
        if (is_valid(x, road_y)) {
            cell(x, road_y).terrain = TerrainCellType::Road;
            cell(x, road_y).tile_id = 4;
        }
    }

    // Add Tiberium fields
    for (int i = 0; i < 6; i++) {
        int cx = std::rand() % width;
        int cy = std::rand() % height;
        if (cell(cx, cy).terrain == TerrainCellType::Water) continue;

        int radius = 2 + std::rand() % 3;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy <= radius * radius) {
                    int nx = cx + dx, ny = cy + dy;
                    if (is_valid(nx, ny) &&
                        cell(nx, ny).terrain != TerrainCellType::Water &&
                        cell(nx, ny).terrain != TerrainCellType::Rock) {
                        auto& c = cell(nx, ny);
                        c.terrain = TerrainCellType::Tiberium;
                        c.tile_id = 5;
                        c.tiberium_value = 4 + std::rand() % 8;
                        c.tiberium_growing = true;
                    }
                }
            }
        }
    }
}

// ============================================================
// Tiberium
// ============================================================

void TileMap::spread_tiberium() {
    // Tiberium grows and spreads to adjacent cells
    std::vector<std::pair<int, int>> new_tiberium;

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            auto& c = cell(x, y);
            if (c.terrain != TerrainCellType::Tiberium) continue;
            if (!c.tiberium_growing) continue;

            // Grow existing
            if (c.tiberium_value < 12) {
                c.tiberium_value++;
            }

            // Chance to spread
            if (std::rand() % 100 < 10) {
                int dir = std::rand() % 4;
                int nx = x + FACING_DX[dir * 2];
                int ny = y + FACING_DY[dir * 2];

                if (is_valid(nx, ny)) {
                    auto& nc = cell(nx, ny);
                    if (nc.terrain == TerrainCellType::Clear ||
                        nc.terrain == TerrainCellType::Sand) {
                        new_tiberium.push_back({nx, ny});
                    }
                }
            }
        }
    }

    for (auto& [nx, ny] : new_tiberium) {
        auto& c = cell(nx, ny);
        c.terrain = TerrainCellType::Tiberium;
        c.tile_id = 5;
        c.tiberium_value = 1;
        c.tiberium_growing = true;
    }
}

int TileMap::get_tiberium_value(int x, int y) const {
    if (!is_valid(x, y)) return 0;
    return cell(x, y).tiberium_value;
}

int TileMap::harvest_tiberium(int x, int y, int amount) {
    if (!is_valid(x, y)) return 0;
    auto& c = cells_[y * width_ + x];
    int harvested = std::min(amount, c.tiberium_value);
    c.tiberium_value -= harvested;

    if (c.tiberium_value <= 0) {
        c.terrain = TerrainCellType::Clear;
        c.tile_id = 0;
        c.tiberium_value = 0;
        c.tiberium_growing = false;
    }

    return harvested;
}

void TileMap::place_tiberium(int x, int y, int value) {
    if (!is_valid(x, y)) return;
    auto& c = cells_[y * width_ + x];
    c.terrain = TerrainCellType::Tiberium;
    c.tile_id = 5;
    c.tiberium_value = value;
    c.tiberium_growing = true;
}

// ============================================================
// Fog of War
// ============================================================

void TileMap::reveal_area(int center_x, int center_y, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int nx = center_x + dx;
                int ny = center_y + dy;
                if (is_valid(nx, ny)) {
                    auto& c = cell(nx, ny);
                    c.explored = true;
                    c.visible = true;
                }
            }
        }
    }
}

void TileMap::clear_visibility() {
    for (auto& c : cells_) {
        c.visible = false;
    }
}

void TileMap::update_visibility() {
    // Called each frame after clearing visibility
    // Each unit/building calls reveal_area for their sight range
}

// ============================================================
// Rendering
// ============================================================

void TileMap::render(Renderer& renderer, const Rect& viewport) {
    if (!renderer_) return;

    // Calculate visible cell range
    int start_x = std::max(0, viewport.x / tile_render_size_);
    int start_y = std::max(0, viewport.y / tile_render_size_);
    int end_x = std::min(width_,
        (viewport.x + viewport.w) / tile_render_size_ + 1);
    int end_y = std::min(height_,
        (viewport.y + viewport.h) / tile_render_size_ + 1);

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            const auto& c = cell(x, y);

            // Terrain color based on type
            Color color;
            switch (c.terrain) {
                case TerrainCellType::Clear:
                    color = Color(180, 160, 120);
                    break;
                case TerrainCellType::Sand:
                    color = Color(210, 190, 140);
                    break;
                case TerrainCellType::Rock:
                    color = Color(100, 100, 100);
                    break;
                case TerrainCellType::Water:
                    color = Color(30, 60, 120);
                    break;
                case TerrainCellType::Road:
                    color = Color(90, 90, 90);
                    break;
                case TerrainCellType::Tiberium: {
                    // Green intensity based on value
                    int g = 100 + std::min(c.tiberium_value * 12, 155);
                    color = Color(0, g, 0);
                    break;
                }
                case TerrainCellType::Rough:
                    color = Color(140, 120, 90);
                    break;
                case TerrainCellType::Cliff:
                    color = Color(80, 70, 60);
                    break;
                default:
                    color = Color(180, 160, 120);
                    break;
            }

            Rect dest(x * tile_render_size_, y * tile_render_size_,
                      tile_render_size_, tile_render_size_);
            renderer.draw_rect_filled(dest, color);
        }
    }
}

void TileMap::render_fog(Renderer& renderer, const Rect& viewport) {
    int start_x = std::max(0, viewport.x / tile_render_size_);
    int start_y = std::max(0, viewport.y / tile_render_size_);
    int end_x = std::min(width_,
        (viewport.x + viewport.w) / tile_render_size_ + 1);
    int end_y = std::min(height_,
        (viewport.y + viewport.h) / tile_render_size_ + 1);

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            const auto& c = cell(x, y);

            if (!c.explored) {
                // Unexplored: fully black
                Rect dest(x * tile_render_size_, y * tile_render_size_,
                          tile_render_size_, tile_render_size_);
                renderer.draw_rect_filled(dest, Color(0, 0, 0, 255));
            } else if (!c.visible) {
                // Explored but not visible: dark overlay
                Rect dest(x * tile_render_size_, y * tile_render_size_,
                          tile_render_size_, tile_render_size_);
                renderer.draw_rect_filled(dest, Color(0, 0, 0, 128));
            }
        }
    }
}

void TileMap::render_grid(Renderer& renderer, const Rect& viewport) {
    int start_x = std::max(0, viewport.x / tile_render_size_);
    int start_y = std::max(0, viewport.y / tile_render_size_);
    int end_x = std::min(width_,
        (viewport.x + viewport.w) / tile_render_size_ + 1);
    int end_y = std::min(height_,
        (viewport.y + viewport.h) / tile_render_size_ + 1);

    Color grid_color(255, 255, 255, 30);

    for (int y = start_y; y <= end_y; y++) {
        int py = y * tile_render_size_;
        renderer.draw_line(start_x * tile_render_size_, py,
                           end_x * tile_render_size_, py, grid_color);
    }
    for (int x = start_x; x <= end_x; x++) {
        int px = x * tile_render_size_;
        renderer.draw_line(px, start_y * tile_render_size_,
                           px, end_y * tile_render_size_, grid_color);
    }
}

void TileMap::render_minimap(Renderer& renderer, const Rect& dest) {
    if (width_ == 0 || height_ == 0) return;

    float scale_x = static_cast<float>(dest.w) / width_;
    float scale_y = static_cast<float>(dest.h) / height_;

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            const auto& c = cell(x, y);

            Color color;
            switch (c.terrain) {
                case TerrainCellType::Clear:
                    color = Color(160, 140, 100);
                    break;
                case TerrainCellType::Sand:
                    color = Color(200, 180, 130);
                    break;
                case TerrainCellType::Rock:
                    color = Color(80, 80, 80);
                    break;
                case TerrainCellType::Water:
                    color = Color(20, 40, 100);
                    break;
                case TerrainCellType::Road:
                    color = Color(70, 70, 70);
                    break;
                case TerrainCellType::Tiberium:
                    color = Color(0, 150, 0);
                    break;
                default:
                    color = Color(160, 140, 100);
                    break;
            }

            Rect pixel(
                dest.x + static_cast<int>(x * scale_x),
                dest.y + static_cast<int>(y * scale_y),
                std::max(1, static_cast<int>(scale_x)),
                std::max(1, static_cast<int>(scale_y))
            );
            renderer.draw_rect_filled(pixel, color);
        }
    }
}

// ============================================================
// MapCell movement helpers
// ============================================================

bool MapCell::passable(SpeedType speed) const {
    switch (terrain) {
        case TerrainCellType::Water:
            return speed == SpeedType::Float || speed == SpeedType::Hover ||
                   speed == SpeedType::Fly;
        case TerrainCellType::Rock:
        case TerrainCellType::Cliff:
            return speed == SpeedType::Fly;
        case TerrainCellType::Wall:
            return speed == SpeedType::Fly;
        default:
            return true;
    }
}

int MapCell::get_movement_cost(SpeedType speed) const {
    if (!passable(speed)) return 9999;

    switch (terrain) {
        case TerrainCellType::Road:
            return (speed == SpeedType::Wheel) ? 1 : 1;
        case TerrainCellType::Sand:
            return (speed == SpeedType::Wheel) ? 3 : 2;
        case TerrainCellType::Rough:
            return 3;
        case TerrainCellType::Tiberium:
            return (speed == SpeedType::Foot) ? 2 : 1;
        default:
            return 1;
    }
}

} // namespace CnC
