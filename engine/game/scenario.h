/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Scenario Loader
 *
 * Defines and loads scenarios (missions / skirmish setups).
 * A scenario specifies:
 *   - Map terrain layout
 *   - Starting units and buildings per player
 *   - Starting credits
 *   - Tiberium field placements
 *   - Win/lose conditions
 *
 * Supports both hardcoded campaign scenarios and JSON-based
 * custom scenarios for skirmish / multiplayer.
 */

#pragma once

#include "game_types.h"
#include "unit.h"
#include "house.h"
#include "../graphics/tilemap.h"
#include <string>
#include <vector>

namespace CnC {

// ============================================================
// Scenario Definition
// ============================================================

struct ScenarioUnit {
    UnitType type = UnitType::None;
    HouseType owner = HouseType::GDI;
    int cell_x = 0;
    int cell_y = 0;
    MissionType mission = MissionType::Guard;
};

struct ScenarioInfantry {
    InfantryType type = InfantryType::None;
    HouseType owner = HouseType::GDI;
    int cell_x = 0;
    int cell_y = 0;
    MissionType mission = MissionType::Guard;
};

struct ScenarioBuilding {
    BuildingType type = BuildingType::None;
    HouseType owner = HouseType::GDI;
    int cell_x = 0;
    int cell_y = 0;
    float health_ratio = 1.0f;    // 0.0 - 1.0
    bool pre_built = true;        // false = under construction
};

struct ScenarioTiberium {
    int cell_x = 0;
    int cell_y = 0;
    int radius = 3;               // Spread radius from center
    int density = 8;              // Max Tiberium value
};

struct ScenarioPlayer {
    HouseType house = HouseType::GDI;
    std::string name;
    bool is_human = false;
    int starting_credits = 5000;
    int starting_power = 0;       // Auto-calculated from buildings

    // Starting base location (for AI base building)
    int base_cell_x = 0;
    int base_cell_y = 0;
};

struct ScenarioDef {
    // Metadata
    std::string name;
    std::string description;
    int scenario_number = 0;

    // Map
    int map_width = 32;
    int map_height = 32;
    std::string map_template;     // "desert", "temperate", etc.

    // Players
    std::vector<ScenarioPlayer> players;

    // Pre-placed entities
    std::vector<ScenarioBuilding> buildings;
    std::vector<ScenarioUnit> units;
    std::vector<ScenarioInfantry> infantry;

    // Tiberium fields
    std::vector<ScenarioTiberium> tiberium_fields;

    // Custom terrain patches (overrides on top of template)
    struct TerrainPatch {
        int x, y;
        int width, height;
        TerrainCellType terrain;
    };
    std::vector<TerrainPatch> terrain_patches;
};

// ============================================================
// Scenario Library
// Built-in scenarios for quick play
// ============================================================

class ScenarioLibrary {
public:
    // Campaign missions
    static ScenarioDef gdi_mission_1();
    static ScenarioDef nod_mission_1();

    // Skirmish templates
    static ScenarioDef skirmish_small();
    static ScenarioDef skirmish_medium();
    static ScenarioDef skirmish_large();

    // Quick test scenario
    static ScenarioDef test_scenario();
};

// ============================================================
// Scenario Loader
// Instantiates a ScenarioDef into the live game objects.
// ============================================================

class ScenarioLoader {
public:
    ScenarioLoader() = default;

    // Load a scenario definition into the game session
    bool load(const ScenarioDef& scenario,
              GameSession& session,
              TileMap& tilemap);

    // Load from a JSON file
    bool load_from_file(const std::string& path,
                        GameSession& session,
                        TileMap& tilemap);

    // Save current game state as scenario (for map editor)
    bool save_to_file(const std::string& path,
                      const GameSession& session,
                      const TileMap& tilemap);

private:
    void generate_terrain(const ScenarioDef& scenario, TileMap& tilemap);
    void apply_terrain_patches(const ScenarioDef& scenario, TileMap& tilemap);
    void place_tiberium(const ScenarioDef& scenario, TileMap& tilemap);
    void setup_players(const ScenarioDef& scenario, GameSession& session);
    void place_buildings(const ScenarioDef& scenario,
                         GameSession& session, TileMap& tilemap);
    void place_units(const ScenarioDef& scenario, GameSession& session);
    void place_infantry(const ScenarioDef& scenario, GameSession& session);
    void setup_fog_of_war(const ScenarioDef& scenario,
                          GameSession& session, TileMap& tilemap);
};

} // namespace CnC
