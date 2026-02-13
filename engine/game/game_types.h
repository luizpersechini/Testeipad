/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Game Types & Data Definitions
 *
 * Ported from the original DEFINES.H, TYPE.H, and related files.
 * These define all the core game enumerations, unit/building types,
 * weapon types, and faction data.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace CnC {

// ============================================================
// Core Constants (from original DEFINES.H)
// ============================================================

// Map dimensions - original C&C uses 64x64 cell maps
constexpr int MAP_MAX_WIDTH = 64;
constexpr int MAP_MAX_HEIGHT = 64;
constexpr int MAP_MAX_CELLS = MAP_MAX_WIDTH * MAP_MAX_HEIGHT;

// Cell dimensions in pixels (original: 24x24)
constexpr int CELL_PIXEL_WIDTH = 24;
constexpr int CELL_PIXEL_HEIGHT = 24;

// Icon/tile size for modern rendering (scaled up)
constexpr int TILE_RENDER_SIZE = 48;

// Maximum counts
constexpr int MAX_PLAYERS = 6;
constexpr int MAX_TEAMS = 16;
constexpr int MAX_UNITS = 500;
constexpr int MAX_BUILDINGS = 500;
constexpr int MAX_INFANTRY = 500;
constexpr int MAX_AIRCRAFT = 100;
constexpr int MAX_BULLETS = 100;
constexpr int MAX_ANIMATIONS = 100;

// Game speed (ticks per second at different speed settings)
constexpr int GAME_SPEEDS[] = {2, 4, 6, 8, 10, 12, 14, 15};

// ============================================================
// Factions / Houses (from HOUSE.H)
// ============================================================

enum class HouseType : int8_t {
    None = -1,
    GDI = 0,           // Global Defense Initiative
    Nod,               // Brotherhood of Nod
    Neutral,           // Neutral/civilian
    Special,           // Special (missions)
    Multi1,            // Multiplayer slots
    Multi2,
    Multi3,
    Multi4,
    Multi5,
    Multi6,
    Count
};

inline const char* house_name(HouseType h) {
    switch (h) {
        case HouseType::GDI:     return "GDI";
        case HouseType::Nod:     return "Nod";
        case HouseType::Neutral: return "Neutral";
        case HouseType::Special: return "Special";
        default:                 return "Multi";
    }
}

// ============================================================
// Unit Types (from original UNIT.H / UDATA.CPP)
// ============================================================

enum class UnitType : int8_t {
    None = -1,
    // GDI Vehicles
    HTANK = 0,     // Mammoth Tank
    MTANK,         // Medium Tank
    LTANK,         // Light Tank
    STANK,         // Stealth Tank
    APC,           // Armored Personnel Carrier
    MLRS,          // Mobile Rocket Launch System (MLRS)
    MSAM,          // Mobile SAM (actually Rocket Launcher)
    HARV,          // Harvester
    MCV,           // Mobile Construction Vehicle
    JEEP,          // Humvee
    BUGGY,         // Nod Buggy
    BIKE,          // Recon Bike
    ARTY,          // Artillery
    STNK,          // Flame Tank (SSM Launcher in some versions)
    FTNK,          // Flame Tank
    BOAT,          // Gunboat
    HOVER,         // Hovercraft
    Count
};

inline const char* unit_name(UnitType u) {
    static const char* names[] = {
        "Mammoth Tank", "Medium Tank", "Light Tank", "Stealth Tank",
        "APC", "MLRS", "Rocket Launcher", "Harvester",
        "MCV", "Humvee", "Nod Buggy", "Recon Bike",
        "Artillery", "SSM Launcher", "Flame Tank",
        "Gunboat", "Hovercraft"
    };
    int idx = static_cast<int>(u);
    if (idx >= 0 && idx < static_cast<int>(UnitType::Count))
        return names[idx];
    return "Unknown";
}

// ============================================================
// Infantry Types (from original INFANTRY.H / IDATA.CPP)
// ============================================================

enum class InfantryType : int8_t {
    None = -1,
    E1 = 0,        // Minigunner
    E2,            // Grenadier
    E3,            // Rocket Soldier
    E4,            // Flamethrower
    E5,            // Chem Warrior
    E6,            // Engineer
    E7,            // Commando
    Count
};

inline const char* infantry_name(InfantryType i) {
    static const char* names[] = {
        "Minigunner", "Grenadier", "Rocket Soldier",
        "Flamethrower", "Chem Warrior", "Engineer", "Commando"
    };
    int idx = static_cast<int>(i);
    if (idx >= 0 && idx < static_cast<int>(InfantryType::Count))
        return names[idx];
    return "Unknown";
}

// ============================================================
// Building Types (from original BUILDING.H / BDATA.CPP)
// ============================================================

enum class BuildingType : int8_t {
    None = -1,
    WEAP = 0,      // Weapons Factory
    GTWR,          // Guard Tower
    ATWR,          // Advanced Guard Tower
    OBELISK,       // Obelisk of Light
    TMPL,          // Temple of Nod
    EYE,           // Advanced Comm Center (GDI)
    HAND,          // Hand of Nod
    AFLD,          // Airfield / Airstrip
    PROC,          // Tiberium Refinery
    SILO,          // Tiberium Silo
    HPAD,          // Helipad
    HQ,            // Construction Yard
    SAM,           // SAM Site
    FACT,          // Barracks (GDI)
    NUKE,          // Power Plant
    NUK2,          // Advanced Power Plant
    GUN,           // Turret
    WALL,          // Concrete Wall
    SBAG,          // Sandbag Wall
    CYCL,          // Chain Link Fence
    BRIK,          // Concrete Barrier
    WOOD,          // Wood Fence
    REPAIR,        // Repair Bay
    Count
};

inline const char* building_name(BuildingType b) {
    static const char* names[] = {
        "Weapons Factory", "Guard Tower", "Advanced Guard Tower",
        "Obelisk of Light", "Temple of Nod", "Advanced Comm Center",
        "Hand of Nod", "Airstrip", "Tiberium Refinery",
        "Tiberium Silo", "Helipad", "Construction Yard",
        "SAM Site", "Barracks", "Power Plant",
        "Advanced Power Plant", "Turret", "Concrete Wall",
        "Sandbag Wall", "Chain Link Fence", "Concrete Barrier",
        "Wood Fence", "Repair Bay"
    };
    int idx = static_cast<int>(b);
    if (idx >= 0 && idx < static_cast<int>(BuildingType::Count))
        return names[idx];
    return "Unknown";
}

// ============================================================
// Aircraft Types (from original AIRCRAFT.H)
// ============================================================

enum class AircraftType : int8_t {
    None = -1,
    TRANSPORT = 0, // Chinook Transport Helicopter
    ORCA,          // Orca Assault Craft
    HELICOPTER,    // Apache Attack Helicopter
    A10,           // A-10 Airstrike
    C17,           // C-17 Cargo Plane (reinforcements)
    Count
};

// ============================================================
// Weapon Types (from original defines)
// ============================================================

enum class WeaponType : int8_t {
    None = -1,
    Machinegun = 0,
    ChainGun,
    Grenade,
    Rocket,
    TurretGun,
    MammothTusk,
    Obelisk,
    Flamethrower,
    ChemSpray,
    Artillery,
    Napalm,
    SSM,
    AirStrike,
    SniperRifle,
    IonCannon,
    NukeStrike,
    Count
};

// ============================================================
// Armor Types
// ============================================================

enum class ArmorType : int8_t {
    None = 0,
    Wood,
    Aluminum,
    Steel,
    Concrete,
    Count
};

// ============================================================
// Movement Types
// ============================================================

enum class SpeedType : int8_t {
    Foot = 0,     // Infantry
    Track,        // Tracked vehicles
    Wheel,        // Wheeled vehicles
    Hover,        // Hovercraft
    Float,        // Boats
    Fly,          // Aircraft
    Count
};

// ============================================================
// Terrain Types (for map cells)
// ============================================================

enum class TerrainCellType : uint8_t {
    Clear = 0,
    Road,
    Water,
    Rock,
    Wall,
    Tiberium,
    Sand,
    Rough,
    River,
    Cliff,
    Bridge,
    Count
};

// ============================================================
// Facing / Direction (8-directional, matching original)
// ============================================================

enum class FacingType : uint8_t {
    North = 0,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
    Count
};

// Direction offsets for 8-directional movement
constexpr int FACING_DX[] = { 0,  1,  1,  1,  0, -1, -1, -1};
constexpr int FACING_DY[] = {-1, -1,  0,  1,  1,  1,  0, -1};

// ============================================================
// Unit/Building Data Templates
// ============================================================

struct WeaponData {
    WeaponType type = WeaponType::None;
    int damage = 0;
    int range = 0;        // In cells
    int rate_of_fire = 0; // Ticks between shots
    std::string projectile_sprite;
    std::string fire_sound;
};

struct UnitTypeData {
    UnitType type = UnitType::None;
    std::string name;
    std::string sprite_base;    // Base sprite name for asset loading
    HouseType owner = HouseType::None; // Which faction can build this

    int hit_points = 100;
    int cost = 0;
    int build_time = 0;         // In game ticks
    int sight_range = 4;        // In cells
    int speed = 4;              // Movement speed
    SpeedType speed_type = SpeedType::Track;
    ArmorType armor = ArmorType::Steel;

    WeaponData primary_weapon;
    WeaponData secondary_weapon;

    bool can_crush_infantry = false;
    bool is_harvester = false;
    bool can_cloak = false;
    int passengers = 0;         // APC capacity

    // Tech tree requirements
    std::vector<BuildingType> prerequisites;
    int tech_level = 1;
};

struct BuildingTypeData {
    BuildingType type = BuildingType::None;
    std::string name;
    std::string sprite_base;
    HouseType owner = HouseType::None;

    int hit_points = 200;
    int cost = 0;
    int build_time = 0;
    int sight_range = 3;
    int power_output = 0;       // Positive = produces, negative = consumes
    ArmorType armor = ArmorType::Concrete;

    WeaponData weapon;          // For defensive buildings

    int width_cells = 2;        // Building footprint
    int height_cells = 2;

    bool has_bib = false;       // Ground apron around building
    bool is_wall = false;
    bool can_repair = false;
    bool produces_units = false;
    bool produces_infantry = false;

    std::vector<BuildingType> prerequisites;
    int tech_level = 1;
};

struct InfantryTypeData {
    InfantryType type = InfantryType::None;
    std::string name;
    std::string sprite_base;
    HouseType owner = HouseType::None;

    int hit_points = 50;
    int cost = 0;
    int build_time = 0;
    int sight_range = 3;
    int speed = 3;
    ArmorType armor = ArmorType::None;

    WeaponData primary_weapon;

    bool is_engineer = false;
    bool is_commando = false;
    bool can_capture = false;
    bool can_c4 = false;

    std::vector<BuildingType> prerequisites;
    int tech_level = 1;
};

// ============================================================
// Game Data Registry
// ============================================================

class GameDataRegistry {
public:
    static GameDataRegistry& instance();

    void init();

    const UnitTypeData& get_unit(UnitType type) const;
    const BuildingTypeData& get_building(BuildingType type) const;
    const InfantryTypeData& get_infantry(InfantryType type) const;

    const std::vector<UnitTypeData>& all_units() const { return units_; }
    const std::vector<BuildingTypeData>& all_buildings() const { return buildings_; }
    const std::vector<InfantryTypeData>& all_infantry() const { return infantry_; }

private:
    GameDataRegistry() = default;
    void init_units();
    void init_buildings();
    void init_infantry();

    std::vector<UnitTypeData> units_;
    std::vector<BuildingTypeData> buildings_;
    std::vector<InfantryTypeData> infantry_;
};

} // namespace CnC
