/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Unit System
 *
 * Ported from the original UNIT.H/UNIT.CPP, TECHNO.H, FOOT.H
 *
 * The original class hierarchy:
 *   AbstractClass -> ObjectClass -> MissionClass -> RadioClass ->
 *   TechnoClass -> FootClass -> DriveClass -> UnitClass
 *
 * This modern port flattens and simplifies the hierarchy while
 * preserving the core gameplay mechanics.
 */

#pragma once

#include "game_types.h"
#include "../graphics/sprite.h"
#include "../core/platform.h"
#include <vector>
#include <memory>
#include <functional>

namespace CnC {

// Forward declarations
class GameMap;
class TileMap;

// ============================================================
// Entity ID System
// ============================================================

using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;

// ============================================================
// Mission / Order State
// (from original MISSION.H - what the unit is currently doing)
// ============================================================

enum class MissionType {
    Sleep,          // Doing nothing
    Guard,          // Guard current position
    GuardArea,      // Guard an area (patrol)
    Move,           // Moving to a destination
    Attack,         // Attacking a target
    Harvest,        // Harvesting Tiberium
    Return,         // Returning to refinery/base
    Repair,         // Moving to repair bay
    Enter,          // Entering a building/transport
    Capture,        // Engineer capturing building
    Hunt,           // Seek and destroy enemies
    Retreat,        // Retreating from combat
    Unload,         // Unloading passengers
    Construct,      // Building under construction
    Sell,           // Building being sold
    Count
};

// ============================================================
// Base Game Entity
// ============================================================

struct GameEntity {
    EntityID id = INVALID_ENTITY;
    HouseType owner = HouseType::None;

    // Position (in pixel coordinates)
    float x = 0.0f;
    float y = 0.0f;

    // Cell position (derived from pixel position)
    int cell_x() const { return static_cast<int>(x) / CELL_PIXEL_WIDTH; }
    int cell_y() const { return static_cast<int>(y) / CELL_PIXEL_HEIGHT; }

    // Health
    int hit_points = 100;
    int max_hit_points = 100;
    bool is_alive() const { return hit_points > 0; }
    float health_ratio() const {
        return max_hit_points > 0 ?
               static_cast<float>(hit_points) / max_hit_points : 0.0f;
    }

    // State
    bool selected = false;
    bool visible = true;

    // Visual
    SpriteInstance sprite;
    int facing = 0;         // 0-7 for 8 directions
    int turret_facing = 0;  // For tanks with separate turret

    // Mission
    MissionType mission = MissionType::Sleep;
};

// ============================================================
// Unit (Vehicle) Entity
// ============================================================

struct Unit : GameEntity {
    UnitType type = UnitType::None;
    const UnitTypeData* type_data = nullptr;

    // Movement
    float move_speed = 0.0f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    bool is_moving = false;
    std::vector<Point> path;       // Current pathfinding result
    int path_index = 0;

    // Combat
    EntityID attack_target = INVALID_ENTITY;
    float fire_cooldown = 0.0f;

    // Harvesting (for harvester units)
    int tiberium_load = 0;
    static constexpr int MAX_TIBERIUM_LOAD = 28;

    // Transport (for APC)
    std::vector<EntityID> passengers;

    // Stealth
    bool cloaked = false;
    bool detected = false;
};

// ============================================================
// Infantry Entity
// ============================================================

struct Infantry : GameEntity {
    InfantryType type = InfantryType::None;
    const InfantryTypeData* type_data = nullptr;

    // Movement
    float move_speed = 0.0f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    bool is_moving = false;
    std::vector<Point> path;
    int path_index = 0;

    // Combat
    EntityID attack_target = INVALID_ENTITY;
    float fire_cooldown = 0.0f;

    // Sub-cell position (infantry share cells, 5 positions per cell)
    int sub_cell = 0;  // 0=center, 1-4=corners

    // Special abilities
    bool prone = false;  // Prone/crawling for damage reduction
};

// ============================================================
// Building Entity
// ============================================================

struct Building : GameEntity {
    BuildingType type = BuildingType::None;
    const BuildingTypeData* type_data = nullptr;

    // Footprint
    int width_cells = 1;
    int height_cells = 1;

    // Production
    bool producing = false;
    float production_progress = 0.0f;  // 0.0 to 1.0
    int producing_unit = -1;           // What's being built
    int producing_infantry = -1;

    // Power
    int power_output = 0;

    // Construction
    bool under_construction = false;
    float construction_progress = 0.0f;

    // Selling
    bool selling = false;
    float sell_progress = 0.0f;

    // Defense weapons
    EntityID attack_target = INVALID_ENTITY;
    float fire_cooldown = 0.0f;
    int turret_facing = 0;

    // Repair
    bool repairing = false;

    // Garrison
    std::vector<EntityID> garrison;
};

// ============================================================
// Projectile Entity
// ============================================================

struct Projectile {
    EntityID id = INVALID_ENTITY;
    WeaponType weapon = WeaponType::None;

    float x, y;
    float target_x, target_y;
    float speed = 5.0f;
    int damage = 10;

    EntityID source = INVALID_ENTITY;
    EntityID target = INVALID_ENTITY;

    SpriteInstance sprite;
    bool active = true;
};

// ============================================================
// Entity Manager
// Manages all game entities with object pooling.
// ============================================================

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    // Creation
    EntityID create_unit(UnitType type, HouseType owner,
                         float x, float y);
    EntityID create_infantry(InfantryType type, HouseType owner,
                             float x, float y);
    EntityID create_building(BuildingType type, HouseType owner,
                             int cell_x, int cell_y);
    EntityID create_projectile(WeaponType weapon,
                               float x, float y,
                               float target_x, float target_y,
                               int damage, EntityID source);

    // Destruction
    void destroy_unit(EntityID id);
    void destroy_infantry(EntityID id);
    void destroy_building(EntityID id);

    // Access
    Unit* get_unit(EntityID id);
    Infantry* get_infantry(EntityID id);
    Building* get_building(EntityID id);

    const std::vector<Unit>& all_units() const { return units_; }
    const std::vector<Infantry>& all_infantry() const { return infantry_; }
    const std::vector<Building>& all_buildings() const { return buildings_; }
    const std::vector<Projectile>& all_projectiles() const {
        return projectiles_;
    }

    std::vector<Unit>& mutable_units() { return units_; }
    std::vector<Infantry>& mutable_infantry() { return infantry_; }
    std::vector<Building>& mutable_buildings() { return buildings_; }
    std::vector<Projectile>& mutable_projectiles() { return projectiles_; }

    // Queries
    std::vector<EntityID> get_units_in_area(const Rect& area) const;
    std::vector<EntityID> get_entities_at_cell(int cx, int cy) const;
    std::vector<EntityID> get_units_by_owner(HouseType owner) const;

    // Selection
    void select_units_in_rect(const Rect& rect, HouseType player);
    void clear_selection();
    std::vector<EntityID> get_selected() const;

    // Update
    void update(float dt, TileMap& map);
    void update_projectiles(float dt);

    // Rendering
    void render(Renderer& renderer, const Rect& viewport);
    void render_health_bars(Renderer& renderer, const Rect& viewport);
    void render_selection_boxes(Renderer& renderer, const Rect& viewport);

private:
    EntityID next_id_ = 1;

    std::vector<Unit> units_;
    std::vector<Infantry> infantry_;
    std::vector<Building> buildings_;
    std::vector<Projectile> projectiles_;

    // ID lookup maps
    std::unordered_map<EntityID, size_t> unit_index_;
    std::unordered_map<EntityID, size_t> infantry_index_;
    std::unordered_map<EntityID, size_t> building_index_;
};

// ============================================================
// Movement & Pathfinding
// ============================================================

class PathFinder {
public:
    // A* pathfinding on the game map
    static std::vector<Point> find_path(
        const TileMap& map,
        int start_x, int start_y,
        int end_x, int end_y,
        SpeedType speed_type
    );

private:
    struct Node {
        int x, y;
        float g_cost, h_cost;
        float f_cost() const { return g_cost + h_cost; }
        int parent_index = -1;
    };
};

// ============================================================
// Combat System
// ============================================================

class CombatSystem {
public:
    // Calculate damage based on weapon vs armor
    static int calculate_damage(WeaponType weapon, ArmorType armor,
                                int base_damage);

    // Check if target is in range
    static bool in_range(float src_x, float src_y,
                         float tgt_x, float tgt_y,
                         int range_cells);

    // Fire weapon from source to target
    static void fire_weapon(EntityManager& entities,
                           EntityID source, EntityID target,
                           const WeaponData& weapon);

    // Process damage on an entity
    static void apply_damage(GameEntity& entity, int damage);

private:
    // Damage multiplier table: weapon_type x armor_type
    // Ported from the original COMBAT.CPP damage tables
    static constexpr float DAMAGE_TABLE[16][5] = {
        // None   Wood   Alum   Steel  Concrete
        { 1.0f,  1.0f,  1.0f,  1.0f,  1.0f  }, // Machinegun
        { 1.0f,  1.0f,  0.8f,  0.4f,  0.3f  }, // ChainGun
        { 1.0f,  1.0f,  0.9f,  0.7f,  0.5f  }, // Grenade
        { 1.0f,  1.0f,  1.0f,  1.0f,  0.8f  }, // Rocket
        { 1.0f,  1.0f,  0.9f,  0.7f,  0.5f  }, // TurretGun
        { 1.0f,  1.0f,  1.0f,  0.9f,  0.8f  }, // MammothTusk
        { 1.0f,  1.2f,  1.2f,  1.0f,  1.0f  }, // Obelisk
        { 1.5f,  1.5f,  1.0f,  0.3f,  0.2f  }, // Flamethrower
        { 1.5f,  1.5f,  1.0f,  0.3f,  0.2f  }, // ChemSpray
        { 1.0f,  1.0f,  1.0f,  0.8f,  0.6f  }, // Artillery
        { 1.5f,  1.5f,  1.0f,  0.4f,  0.3f  }, // Napalm
        { 1.0f,  1.0f,  1.0f,  1.0f,  0.9f  }, // SSM
        { 1.0f,  1.0f,  1.0f,  1.0f,  0.8f  }, // AirStrike
        { 2.0f,  2.0f,  2.0f,  0.1f,  0.1f  }, // SniperRifle
        { 1.0f,  1.0f,  1.0f,  1.0f,  1.0f  }, // IonCannon
        { 1.0f,  1.0f,  1.0f,  1.0f,  1.0f  }, // NukeStrike
    };
};

} // namespace CnC
