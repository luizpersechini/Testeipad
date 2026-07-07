/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Scenario Loader Implementation
 */

#include "scenario.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace CnC {

// ============================================================
// ScenarioLibrary - Built-in scenarios
// ============================================================

ScenarioDef ScenarioLibrary::gdi_mission_1() {
    ScenarioDef s;
    s.name = "GDI Mission 1: The Beachhead";
    s.description = "Establish a base and destroy the Nod outpost.";
    s.scenario_number = 1;
    s.map_width = 48;
    s.map_height = 48;
    s.map_template = "desert";

    // GDI player
    ScenarioPlayer gdi;
    gdi.house = HouseType::GDI;
    gdi.name = "GDI";
    gdi.is_human = true;
    gdi.starting_credits = 8000;
    gdi.base_cell_x = 8;
    gdi.base_cell_y = 40;
    s.players.push_back(gdi);

    // Nod AI
    ScenarioPlayer nod;
    nod.house = HouseType::Nod;
    nod.name = "Nod";
    nod.is_human = false;
    nod.starting_credits = 10000;
    nod.base_cell_x = 38;
    nod.base_cell_y = 8;
    s.players.push_back(nod);

    // GDI starting base
    s.buildings.push_back({BuildingType::HQ,   HouseType::GDI, 8,  40});
    s.buildings.push_back({BuildingType::NUKE, HouseType::GDI, 12, 40});
    s.buildings.push_back({BuildingType::FACT, HouseType::GDI, 6,  38});
    s.buildings.push_back({BuildingType::PROC, HouseType::GDI, 12, 38});

    // GDI starting units
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 10, 42});
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 12, 42});
    s.units.push_back({UnitType::JEEP,  HouseType::GDI, 14, 42});
    s.units.push_back({UnitType::HARV,  HouseType::GDI, 14, 38});
    s.units.push_back({UnitType::MCV,   HouseType::GDI, 8,  44});

    // GDI infantry
    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 9, 41});
    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 10, 41});
    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 11, 41});
    s.infantry.push_back({InfantryType::E3, HouseType::GDI, 7, 41});
    s.infantry.push_back({InfantryType::E3, HouseType::GDI, 13, 41});

    // Nod base
    s.buildings.push_back({BuildingType::HQ,      HouseType::Nod, 38, 8});
    s.buildings.push_back({BuildingType::NUKE,     HouseType::Nod, 36, 8});
    s.buildings.push_back({BuildingType::HAND,     HouseType::Nod, 40, 6});
    s.buildings.push_back({BuildingType::AFLD,     HouseType::Nod, 36, 6});
    s.buildings.push_back({BuildingType::PROC,     HouseType::Nod, 34, 8});
    s.buildings.push_back({BuildingType::GUN,      HouseType::Nod, 34, 10});
    s.buildings.push_back({BuildingType::GUN,      HouseType::Nod, 42, 10});
    s.buildings.push_back({BuildingType::OBELISK,  HouseType::Nod, 38, 12});

    // Nod units
    s.units.push_back({UnitType::LTANK, HouseType::Nod, 36, 10});
    s.units.push_back({UnitType::LTANK, HouseType::Nod, 40, 10});
    s.units.push_back({UnitType::BUGGY, HouseType::Nod, 38, 14});
    s.units.push_back({UnitType::BIKE,  HouseType::Nod, 40, 14});
    s.units.push_back({UnitType::HARV,  HouseType::Nod, 34, 6});

    // Nod infantry
    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 37, 9});
    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 39, 9});
    s.infantry.push_back({InfantryType::E4, HouseType::Nod, 41, 9});
    s.infantry.push_back({InfantryType::E3, HouseType::Nod, 35, 11});

    // Tiberium fields
    s.tiberium_fields.push_back({14, 30, 5, 10});  // Near GDI
    s.tiberium_fields.push_back({30, 14, 5, 10});  // Near Nod
    s.tiberium_fields.push_back({24, 24, 4, 8});   // Center

    // Terrain features
    // River running diagonally
    for (int i = 0; i < 20; i++) {
        s.terrain_patches.push_back({18 + i, 28 - i, 2, 1, TerrainCellType::Water});
    }
    // Rocky outcrops
    s.terrain_patches.push_back({20, 18, 4, 3, TerrainCellType::Rock});
    s.terrain_patches.push_back({26, 30, 3, 4, TerrainCellType::Rock});
    // Roads
    s.terrain_patches.push_back({8, 36, 1, 4, TerrainCellType::Road});
    s.terrain_patches.push_back({38, 4, 1, 4, TerrainCellType::Road});

    return s;
}

ScenarioDef ScenarioLibrary::nod_mission_1() {
    ScenarioDef s;
    s.name = "Nod Mission 1: Silencing Dissent";
    s.description = "Destroy the GDI communications center.";
    s.scenario_number = 1;
    s.map_width = 40;
    s.map_height = 40;
    s.map_template = "desert";

    // Nod player
    ScenarioPlayer nod;
    nod.house = HouseType::Nod;
    nod.name = "Nod";
    nod.is_human = true;
    nod.starting_credits = 6000;
    nod.base_cell_x = 32;
    nod.base_cell_y = 32;
    s.players.push_back(nod);

    // GDI AI
    ScenarioPlayer gdi;
    gdi.house = HouseType::GDI;
    gdi.name = "GDI";
    gdi.is_human = false;
    gdi.starting_credits = 8000;
    gdi.base_cell_x = 6;
    gdi.base_cell_y = 6;
    s.players.push_back(gdi);

    // Nod starting forces (no base - commando mission style)
    s.units.push_back({UnitType::BUGGY, HouseType::Nod, 34, 34});
    s.units.push_back({UnitType::BUGGY, HouseType::Nod, 36, 34});
    s.units.push_back({UnitType::BIKE,  HouseType::Nod, 35, 36});

    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 33, 33});
    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 34, 33});
    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 35, 33});
    s.infantry.push_back({InfantryType::E4, HouseType::Nod, 33, 35});
    s.infantry.push_back({InfantryType::E3, HouseType::Nod, 36, 33});

    // GDI base
    s.buildings.push_back({BuildingType::HQ,   HouseType::GDI, 6,  6});
    s.buildings.push_back({BuildingType::NUKE, HouseType::GDI, 4,  6});
    s.buildings.push_back({BuildingType::FACT, HouseType::GDI, 8,  4});
    s.buildings.push_back({BuildingType::GTWR, HouseType::GDI, 10, 8});
    s.buildings.push_back({BuildingType::GTWR, HouseType::GDI, 4,  10});
    s.buildings.push_back({BuildingType::EYE,  HouseType::GDI, 6,  2});  // Comm center (target)

    s.units.push_back({UnitType::MTANK, HouseType::GDI, 8, 8});
    s.units.push_back({UnitType::JEEP,  HouseType::GDI, 10, 6});

    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 5, 5});
    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 7, 5});
    s.infantry.push_back({InfantryType::E3, HouseType::GDI, 9, 5});

    // Tiberium
    s.tiberium_fields.push_back({20, 20, 4, 10});  // Center field

    return s;
}

ScenarioDef ScenarioLibrary::skirmish_small() {
    ScenarioDef s;
    s.name = "Skirmish: Desert Clash";
    s.description = "Small 2-player desert skirmish map.";
    s.map_width = 32;
    s.map_height = 32;
    s.map_template = "desert";

    // Player 1
    ScenarioPlayer p1;
    p1.house = HouseType::GDI;
    p1.name = "Player 1";
    p1.is_human = true;
    p1.starting_credits = 10000;
    p1.base_cell_x = 4;
    p1.base_cell_y = 26;
    s.players.push_back(p1);

    // Player 2 (AI)
    ScenarioPlayer p2;
    p2.house = HouseType::Nod;
    p2.name = "Computer";
    p2.is_human = false;
    p2.starting_credits = 10000;
    p2.base_cell_x = 26;
    p2.base_cell_y = 4;
    s.players.push_back(p2);

    // Player 1 starting base
    s.buildings.push_back({BuildingType::HQ,   HouseType::GDI, 4,  26});
    s.buildings.push_back({BuildingType::NUKE, HouseType::GDI, 8,  26});
    s.buildings.push_back({BuildingType::PROC, HouseType::GDI, 4,  24});

    s.units.push_back({UnitType::HARV, HouseType::GDI, 6, 24});
    s.units.push_back({UnitType::MCV,  HouseType::GDI, 4, 28});

    // Player 2 starting base
    s.buildings.push_back({BuildingType::HQ,   HouseType::Nod, 26, 4});
    s.buildings.push_back({BuildingType::NUKE, HouseType::Nod, 24, 4});
    s.buildings.push_back({BuildingType::PROC, HouseType::Nod, 26, 6});

    s.units.push_back({UnitType::HARV, HouseType::Nod, 28, 6});
    s.units.push_back({UnitType::MCV,  HouseType::Nod, 26, 2});

    // Tiberium
    s.tiberium_fields.push_back({6, 20, 3, 10});   // Near P1
    s.tiberium_fields.push_back({24, 10, 3, 10});  // Near P2
    s.tiberium_fields.push_back({16, 16, 4, 12});  // Center

    // Terrain
    s.terrain_patches.push_back({14, 14, 4, 4, TerrainCellType::Rock});
    s.terrain_patches.push_back({0, 14, 3, 2, TerrainCellType::Water});
    s.terrain_patches.push_back({29, 14, 3, 2, TerrainCellType::Water});

    return s;
}

ScenarioDef ScenarioLibrary::skirmish_medium() {
    ScenarioDef s;
    s.name = "Skirmish: Tiberium Valley";
    s.description = "Medium 2-player map with rich Tiberium deposits.";
    s.map_width = 48;
    s.map_height = 48;
    s.map_template = "desert";

    // Player 1
    ScenarioPlayer p1;
    p1.house = HouseType::GDI;
    p1.name = "Player 1";
    p1.is_human = true;
    p1.starting_credits = 10000;
    p1.base_cell_x = 6;
    p1.base_cell_y = 40;
    s.players.push_back(p1);

    // Player 2
    ScenarioPlayer p2;
    p2.house = HouseType::Nod;
    p2.name = "Computer";
    p2.is_human = false;
    p2.starting_credits = 10000;
    p2.base_cell_x = 40;
    p2.base_cell_y = 6;
    s.players.push_back(p2);

    // P1 base
    s.buildings.push_back({BuildingType::HQ,   HouseType::GDI, 6,  40});
    s.buildings.push_back({BuildingType::NUKE, HouseType::GDI, 10, 40});
    s.buildings.push_back({BuildingType::PROC, HouseType::GDI, 6,  38});
    s.buildings.push_back({BuildingType::FACT, HouseType::GDI, 10, 38});

    s.units.push_back({UnitType::HARV,  HouseType::GDI, 8, 38});
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 8, 42});
    s.units.push_back({UnitType::JEEP,  HouseType::GDI, 12, 42});

    // P2 base
    s.buildings.push_back({BuildingType::HQ,   HouseType::Nod, 40, 6});
    s.buildings.push_back({BuildingType::NUKE, HouseType::Nod, 38, 6});
    s.buildings.push_back({BuildingType::PROC, HouseType::Nod, 40, 8});
    s.buildings.push_back({BuildingType::HAND, HouseType::Nod, 38, 8});

    s.units.push_back({UnitType::HARV,  HouseType::Nod, 42, 8});
    s.units.push_back({UnitType::LTANK, HouseType::Nod, 42, 4});
    s.units.push_back({UnitType::BUGGY, HouseType::Nod, 36, 4});

    // Tiberium - multiple fields
    s.tiberium_fields.push_back({8, 32, 4, 10});   // Near P1
    s.tiberium_fields.push_back({38, 14, 4, 10});  // Near P2
    s.tiberium_fields.push_back({22, 22, 6, 12});  // Center big field
    s.tiberium_fields.push_back({14, 14, 3, 8});   // North expansion
    s.tiberium_fields.push_back({34, 34, 3, 8});   // South expansion

    // River through center
    for (int i = 0; i < 24; i++) {
        s.terrain_patches.push_back({10 + i, 22 + (i % 3 == 0 ? 1 : 0), 1, 2, TerrainCellType::Water});
    }
    // Bridges
    s.terrain_patches.push_back({18, 22, 2, 2, TerrainCellType::Bridge});
    s.terrain_patches.push_back({30, 22, 2, 2, TerrainCellType::Bridge});

    // Rocky areas
    s.terrain_patches.push_back({16, 30, 5, 3, TerrainCellType::Rock});
    s.terrain_patches.push_back({28, 16, 3, 5, TerrainCellType::Rock});

    return s;
}

ScenarioDef ScenarioLibrary::skirmish_large() {
    ScenarioDef s;
    s.name = "Skirmish: The Wasteland";
    s.description = "Large map with scarce resources - fight for control.";
    s.map_width = 64;
    s.map_height = 64;
    s.map_template = "desert";

    // Player 1
    ScenarioPlayer p1;
    p1.house = HouseType::GDI;
    p1.name = "Player 1";
    p1.is_human = true;
    p1.starting_credits = 12000;
    p1.base_cell_x = 6;
    p1.base_cell_y = 56;
    s.players.push_back(p1);

    // Player 2
    ScenarioPlayer p2;
    p2.house = HouseType::Nod;
    p2.name = "Computer";
    p2.is_human = false;
    p2.starting_credits = 12000;
    p2.base_cell_x = 56;
    p2.base_cell_y = 6;
    s.players.push_back(p2);

    // P1 base
    s.buildings.push_back({BuildingType::HQ,   HouseType::GDI, 6,  56});
    s.buildings.push_back({BuildingType::NUKE, HouseType::GDI, 10, 56});
    s.buildings.push_back({BuildingType::NUK2, HouseType::GDI, 10, 54});
    s.buildings.push_back({BuildingType::PROC, HouseType::GDI, 6,  54});
    s.buildings.push_back({BuildingType::FACT, HouseType::GDI, 6,  52});
    s.buildings.push_back({BuildingType::WEAP, HouseType::GDI, 10, 52});

    s.units.push_back({UnitType::HARV,  HouseType::GDI, 8, 54});
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 6, 58});
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 8, 58});
    s.units.push_back({UnitType::JEEP,  HouseType::GDI, 12, 58});
    s.units.push_back({UnitType::APC,   HouseType::GDI, 14, 58});

    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 7,  57});
    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 9,  57});
    s.infantry.push_back({InfantryType::E3, HouseType::GDI, 11, 57});

    // P2 base
    s.buildings.push_back({BuildingType::HQ,   HouseType::Nod, 56, 6});
    s.buildings.push_back({BuildingType::NUKE, HouseType::Nod, 54, 6});
    s.buildings.push_back({BuildingType::NUK2, HouseType::Nod, 54, 8});
    s.buildings.push_back({BuildingType::PROC, HouseType::Nod, 56, 8});
    s.buildings.push_back({BuildingType::HAND, HouseType::Nod, 56, 10});
    s.buildings.push_back({BuildingType::AFLD, HouseType::Nod, 52, 6});

    s.units.push_back({UnitType::HARV,  HouseType::Nod, 58, 8});
    s.units.push_back({UnitType::LTANK, HouseType::Nod, 56, 4});
    s.units.push_back({UnitType::LTANK, HouseType::Nod, 58, 4});
    s.units.push_back({UnitType::BUGGY, HouseType::Nod, 54, 4});
    s.units.push_back({UnitType::BIKE,  HouseType::Nod, 52, 4});

    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 55, 5});
    s.infantry.push_back({InfantryType::E4, HouseType::Nod, 57, 5});
    s.infantry.push_back({InfantryType::E3, HouseType::Nod, 53, 5});

    // Sparse Tiberium fields across the map
    s.tiberium_fields.push_back({8,  48, 3, 8});
    s.tiberium_fields.push_back({54, 14, 3, 8});
    s.tiberium_fields.push_back({30, 30, 5, 12});  // Central large field
    s.tiberium_fields.push_back({20, 44, 3, 6});
    s.tiberium_fields.push_back({44, 20, 3, 6});
    s.tiberium_fields.push_back({14, 24, 2, 6});   // Contested expansions
    s.tiberium_fields.push_back({48, 40, 2, 6});

    // Major river system
    for (int i = 0; i < 40; i++) {
        int wx = 12 + i;
        int wy = 30 + static_cast<int>(3.0f * std::sin(i * 0.3f));
        s.terrain_patches.push_back({wx, wy, 1, 2, TerrainCellType::Water});
    }
    // Bridges
    s.terrain_patches.push_back({22, 30, 2, 2, TerrainCellType::Bridge});
    s.terrain_patches.push_back({38, 30, 2, 2, TerrainCellType::Bridge});

    // Mountain ranges
    s.terrain_patches.push_back({24, 16, 8, 3, TerrainCellType::Cliff});
    s.terrain_patches.push_back({30, 44, 8, 3, TerrainCellType::Cliff});
    s.terrain_patches.push_back({8,  32, 3, 6, TerrainCellType::Rock});
    s.terrain_patches.push_back({52, 28, 3, 6, TerrainCellType::Rock});

    return s;
}

ScenarioDef ScenarioLibrary::test_scenario() {
    ScenarioDef s;
    s.name = "Test Scenario";
    s.description = "Quick test with small forces.";
    s.map_width = 32;
    s.map_height = 32;
    s.map_template = "desert";

    // Player
    ScenarioPlayer p1;
    p1.house = HouseType::GDI;
    p1.name = "Player";
    p1.is_human = true;
    p1.starting_credits = 20000;
    p1.base_cell_x = 4;
    p1.base_cell_y = 24;
    s.players.push_back(p1);

    // AI
    ScenarioPlayer p2;
    p2.house = HouseType::Nod;
    p2.name = "AI";
    p2.is_human = false;
    p2.starting_credits = 10000;
    p2.base_cell_x = 24;
    p2.base_cell_y = 4;
    s.players.push_back(p2);

    // Player base
    s.buildings.push_back({BuildingType::HQ,   HouseType::GDI, 4,  24});
    s.buildings.push_back({BuildingType::NUKE, HouseType::GDI, 8,  24});
    s.buildings.push_back({BuildingType::PROC, HouseType::GDI, 4,  22});
    s.buildings.push_back({BuildingType::FACT, HouseType::GDI, 8,  22});
    s.buildings.push_back({BuildingType::WEAP, HouseType::GDI, 4,  20});

    s.units.push_back({UnitType::HARV,  HouseType::GDI, 6, 22});
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 4, 26});
    s.units.push_back({UnitType::MTANK, HouseType::GDI, 6, 26});
    s.units.push_back({UnitType::JEEP,  HouseType::GDI, 8, 26});

    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 5, 25});
    s.infantry.push_back({InfantryType::E1, HouseType::GDI, 7, 25});
    s.infantry.push_back({InfantryType::E3, HouseType::GDI, 9, 25});

    // AI base
    s.buildings.push_back({BuildingType::HQ,   HouseType::Nod, 24, 4});
    s.buildings.push_back({BuildingType::NUKE, HouseType::Nod, 22, 4});
    s.buildings.push_back({BuildingType::HAND, HouseType::Nod, 24, 6});
    s.buildings.push_back({BuildingType::PROC, HouseType::Nod, 22, 6});

    s.units.push_back({UnitType::LTANK, HouseType::Nod, 24, 2});
    s.units.push_back({UnitType::BUGGY, HouseType::Nod, 26, 2});
    s.units.push_back({UnitType::HARV,  HouseType::Nod, 22, 8});

    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 23, 3});
    s.infantry.push_back({InfantryType::E1, HouseType::Nod, 25, 3});

    // Tiberium
    s.tiberium_fields.push_back({6,  16, 3, 10});
    s.tiberium_fields.push_back({22, 12, 3, 10});
    s.tiberium_fields.push_back({14, 14, 3, 8});

    // Some terrain
    s.terrain_patches.push_back({14, 10, 3, 3, TerrainCellType::Rock});
    s.terrain_patches.push_back({10, 18, 2, 4, TerrainCellType::Rock});

    return s;
}

// ============================================================
// ScenarioLoader Implementation
// ============================================================

bool ScenarioLoader::load(const ScenarioDef& scenario,
                          GameSession& session,
                          TileMap& tilemap) {
    // 1. Generate terrain
    generate_terrain(scenario, tilemap);

    // 2. Apply terrain patches (rivers, rocks, roads, etc.)
    apply_terrain_patches(scenario, tilemap);

    // 3. Place Tiberium fields
    place_tiberium(scenario, tilemap);

    // 4. Setup players/houses
    setup_players(scenario, session);

    // 5. Place pre-built buildings
    place_buildings(scenario, session, tilemap);

    // 6. Place starting units
    place_units(scenario, session);

    // 7. Place starting infantry
    place_infantry(scenario, session);

    // 8. Set initial fog of war
    setup_fog_of_war(scenario, session, tilemap);

    return true;
}

void ScenarioLoader::generate_terrain(const ScenarioDef& scenario,
                                       TileMap& tilemap) {
    tilemap.create_empty(scenario.map_width, scenario.map_height);

    std::mt19937 rng(42); // Deterministic seed for reproducibility
    std::uniform_int_distribution<int> terrain_noise(0, 100);

    for (int y = 0; y < scenario.map_height; y++) {
        for (int x = 0; x < scenario.map_width; x++) {
            MapCell& c = tilemap.cell(x, y);

            if (scenario.map_template == "desert") {
                // Desert template: mostly sand with scattered rough terrain
                int noise = terrain_noise(rng);
                if (noise < 75) {
                    c.terrain = TerrainCellType::Sand;
                } else if (noise < 90) {
                    c.terrain = TerrainCellType::Clear;
                } else {
                    c.terrain = TerrainCellType::Rough;
                }
            } else if (scenario.map_template == "temperate") {
                int noise = terrain_noise(rng);
                if (noise < 70) {
                    c.terrain = TerrainCellType::Clear;
                } else if (noise < 85) {
                    c.terrain = TerrainCellType::Rough;
                } else {
                    c.terrain = TerrainCellType::Sand;
                }
            } else {
                // Default: clear terrain
                c.terrain = TerrainCellType::Clear;
            }

            // Map edges are impassable cliffs
            if (x == 0 || y == 0 ||
                x == scenario.map_width - 1 || y == scenario.map_height - 1) {
                c.terrain = TerrainCellType::Cliff;
            }
        }
    }
}

void ScenarioLoader::apply_terrain_patches(const ScenarioDef& scenario,
                                            TileMap& tilemap) {
    for (const auto& patch : scenario.terrain_patches) {
        for (int dy = 0; dy < patch.height; dy++) {
            for (int dx = 0; dx < patch.width; dx++) {
                int px = patch.x + dx;
                int py = patch.y + dy;
                if (tilemap.is_valid(px, py)) {
                    tilemap.cell(px, py).terrain = patch.terrain;
                }
            }
        }
    }
}

void ScenarioLoader::place_tiberium(const ScenarioDef& scenario,
                                     TileMap& tilemap) {
    std::mt19937 rng(123);

    for (const auto& field : scenario.tiberium_fields) {
        for (int dy = -field.radius; dy <= field.radius; dy++) {
            for (int dx = -field.radius; dx <= field.radius; dx++) {
                int px = field.cell_x + dx;
                int py = field.cell_y + dy;

                if (!tilemap.is_valid(px, py)) continue;

                MapCell& c = tilemap.cell(px, py);

                // Only place Tiberium on passable, non-water terrain
                if (c.terrain == TerrainCellType::Water ||
                    c.terrain == TerrainCellType::Cliff ||
                    c.terrain == TerrainCellType::Rock) continue;

                // Calculate distance falloff
                float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy));
                if (dist > field.radius) continue;

                float ratio = 1.0f - (dist / static_cast<float>(field.radius));
                int value = static_cast<int>(field.density * ratio);

                // Add some randomness
                std::uniform_int_distribution<int> jitter(-2, 2);
                value += jitter(rng);
                value = std::clamp(value, 1, 12);

                tilemap.place_tiberium(px, py, value);
            }
        }
    }
}

void ScenarioLoader::setup_players(const ScenarioDef& scenario,
                                    GameSession& session) {
    session.init(static_cast<int>(scenario.players.size()));

    for (const auto& sp : scenario.players) {
        House& house = session.get_house(sp.house);
        house.init(sp.house, sp.name, sp.is_human);
        house.set_credits(sp.starting_credits);
    }
}

void ScenarioLoader::place_buildings(const ScenarioDef& scenario,
                                      GameSession& session,
                                      TileMap& tilemap) {
    auto& entities = session.entities();

    for (const auto& sb : scenario.buildings) {
        EntityID id = entities.create_building(sb.type, sb.owner,
                                                sb.cell_x, sb.cell_y);
        Building* bld = entities.get_building(id);
        if (bld) {
            if (!sb.pre_built) {
                bld->under_construction = true;
                bld->construction_progress = 0.0f;
            }
            if (sb.health_ratio < 1.0f) {
                bld->hit_points = static_cast<int>(bld->max_hit_points * sb.health_ratio);
            }

            // Register with owning house
            House& house = session.get_house(sb.owner);
            house.add_building(id);

            // Mark cells as occupied
            const auto& data = GameDataRegistry::instance().get_building(sb.type);
            for (int dy = 0; dy < data.height_cells; dy++) {
                for (int dx = 0; dx < data.width_cells; dx++) {
                    int cx = sb.cell_x + dx;
                    int cy = sb.cell_y + dy;
                    if (tilemap.is_valid(cx, cy)) {
                        tilemap.cell(cx, cy).occupant_building_id = static_cast<int>(id);
                    }
                }
            }
        }
    }
}

void ScenarioLoader::place_units(const ScenarioDef& scenario,
                                  GameSession& session) {
    auto& entities = session.entities();

    for (const auto& su : scenario.units) {
        float px = static_cast<float>(su.cell_x * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2);
        float py = static_cast<float>(su.cell_y * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2);

        EntityID id = entities.create_unit(su.type, su.owner, px, py);
        Unit* unit = entities.get_unit(id);
        if (unit) {
            unit->mission = su.mission;

            // Register with house
            House& house = session.get_house(su.owner);
            house.add_unit(id);
        }
    }
}

void ScenarioLoader::place_infantry(const ScenarioDef& scenario,
                                     GameSession& session) {
    auto& entities = session.entities();

    for (const auto& si : scenario.infantry) {
        float px = static_cast<float>(si.cell_x * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2);
        float py = static_cast<float>(si.cell_y * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2);

        EntityID id = entities.create_infantry(si.type, si.owner, px, py);
        Infantry* inf = entities.get_infantry(id);
        if (inf) {
            inf->mission = si.mission;

            // Register with house
            House& house = session.get_house(si.owner);
            house.add_unit(id);
        }
    }
}

void ScenarioLoader::setup_fog_of_war(const ScenarioDef& scenario,
                                       GameSession& session,
                                       TileMap& tilemap) {
    // Start with everything hidden
    tilemap.clear_visibility();

    // Reveal area around each player's buildings and units
    auto& entities = session.entities();

    for (const auto& bld : entities.all_buildings()) {
        const auto& data = GameDataRegistry::instance().get_building(bld.type);
        tilemap.reveal_area(bld.cell_x() + data.width_cells / 2,
                           bld.cell_y() + data.height_cells / 2,
                           data.sight_range);
    }

    for (const auto& unit : entities.all_units()) {
        if (unit.type_data) {
            tilemap.reveal_area(unit.cell_x(), unit.cell_y(),
                               unit.type_data->sight_range);
        }
    }

    for (const auto& inf : entities.all_infantry()) {
        if (inf.type_data) {
            tilemap.reveal_area(inf.cell_x(), inf.cell_y(),
                               inf.type_data->sight_range);
        }
    }
}

bool ScenarioLoader::load_from_file(const std::string& /*path*/,
                                     GameSession& /*session*/,
                                     TileMap& /*tilemap*/) {
    // TODO: Implement JSON scenario loading
    // Would parse a JSON file into ScenarioDef then call load()
    return false;
}

bool ScenarioLoader::save_to_file(const std::string& /*path*/,
                                   const GameSession& /*session*/,
                                   const TileMap& /*tilemap*/) {
    // TODO: Implement JSON scenario saving
    return false;
}

} // namespace CnC
