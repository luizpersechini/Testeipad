/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Tile Map System
 *
 * Manages the game map composed of terrain tiles.
 * Original C&C uses a 64x64 grid of 24x24 pixel cells.
 * This system handles tile rendering, terrain properties,
 * and Tiberium field management.
 */

#pragma once

#include "../core/platform.h"
#include "../game/game_types.h"
#include <vector>
#include <string>
#include <memory>

namespace CnC {

// ============================================================
// Tile Set Definition
// ============================================================

struct TileDef {
    int tile_id = 0;
    std::string name;
    TerrainCellType terrain_type = TerrainCellType::Clear;
    TextureID texture = INVALID_TEXTURE;
    Rect source_rect;

    // Terrain properties
    bool passable_foot = true;
    bool passable_vehicle = true;
    bool passable_boat = false;
    bool buildable = true;
    int movement_cost = 1;     // Higher = slower movement
    float defense_bonus = 0.0f; // Cover bonus for infantry
};

struct TileSet {
    std::string name;
    TextureID atlas_texture = INVALID_TEXTURE;
    int tile_width = 0;
    int tile_height = 0;
    std::vector<TileDef> tiles;

    const TileDef& get_tile(int id) const {
        if (id >= 0 && id < static_cast<int>(tiles.size()))
            return tiles[id];
        return tiles[0]; // Default tile
    }
};

// ============================================================
// Map Cell
// ============================================================

struct MapCell {
    // Terrain
    int tile_id = 0;                    // Index into tileset
    TerrainCellType terrain = TerrainCellType::Clear;
    int overlay_tile_id = -1;           // Overlay (roads, walls, etc.)

    // Tiberium
    int tiberium_value = 0;             // 0 = none, up to 12 levels
    bool tiberium_growing = false;

    // Fog of war
    bool explored = false;              // Has been seen
    bool visible = false;               // Currently visible

    // Occupancy
    int occupant_unit_id = -1;
    int occupant_building_id = -1;
    int occupant_infantry_id = -1;      // Up to 5 infantry per cell
    bool is_occupied() const {
        return occupant_unit_id >= 0 ||
               occupant_building_id >= 0 ||
               occupant_infantry_id >= 0;
    }

    // Pathfinding
    bool passable(SpeedType speed) const;
    int get_movement_cost(SpeedType speed) const;
};

// ============================================================
// Tile Map
// ============================================================

class TileMap {
public:
    TileMap();
    ~TileMap();

    // Initialization
    bool init(Renderer& renderer, const std::string& tileset_path);
    void shutdown();

    // Map creation
    void create_empty(int width, int height);
    bool load_from_file(const std::string& path);
    bool save_to_file(const std::string& path) const;

    // Generate a default map for testing
    void generate_test_map(int width, int height);

    // Cell access
    MapCell& cell(int x, int y);
    const MapCell& cell(int x, int y) const;
    MapCell& cell_at(int index);
    bool is_valid(int x, int y) const;

    // Coordinate conversion
    Point cell_to_pixel(int cell_x, int cell_y) const;
    Point pixel_to_cell(int pixel_x, int pixel_y) const;
    int cell_to_index(int x, int y) const { return y * width_ + x; }
    Point index_to_cell(int index) const {
        return Point(index % width_, index / width_);
    }

    // Map properties
    int width() const { return width_; }
    int height() const { return height_; }
    int tile_size() const { return tile_render_size_; }

    // Tiberium
    void spread_tiberium();
    int get_tiberium_value(int x, int y) const;
    int harvest_tiberium(int x, int y, int amount);
    void place_tiberium(int x, int y, int value);

    // Fog of war
    void reveal_area(int center_x, int center_y, int radius);
    void update_visibility();
    void clear_visibility();

    // Rendering
    void render(Renderer& renderer, const Rect& viewport);
    void render_fog(Renderer& renderer, const Rect& viewport);
    void render_grid(Renderer& renderer, const Rect& viewport);

    // Tile set management
    void set_tileset(const TileSet& tileset);
    const TileSet& get_tileset() const { return tileset_; }

    // Minimap
    void render_minimap(Renderer& renderer, const Rect& dest);

private:
    int width_ = 0;
    int height_ = 0;
    int tile_render_size_ = TILE_RENDER_SIZE;
    std::vector<MapCell> cells_;
    TileSet tileset_;
    Renderer* renderer_ = nullptr;

    // Tiberium growth timer
    float tiberium_timer_ = 0.0f;
    static constexpr float TIBERIUM_GROW_INTERVAL = 5.0f;

    // Default empty cell for out-of-bounds access
    static MapCell null_cell_;
};

} // namespace CnC
