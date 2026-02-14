/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * House (Player/Faction) System
 *
 * Manages the player's economy, power grid, tech tree, build queue,
 * and superweapons. Ported from the original HOUSE.H/HOUSE.CPP.
 *
 * Each "House" represents a player in the game (human or AI).
 */

#pragma once

#include "game_types.h"
#include "unit.h"
#include <vector>
#include <queue>
#include <string>
#include <functional>

namespace CnC {

// ============================================================
// Build Queue Entry
// ============================================================

enum class ProductionCategory {
    Structure,
    Unit,
    Infantry
};

struct BuildQueueEntry {
    ProductionCategory category;
    int type_index;              // UnitType, InfantryType, or BuildingType
    float progress = 0.0f;       // 0.0 to 1.0
    float build_time = 0.0f;     // Total time in seconds
    int cost = 0;
    bool paused = false;
    bool ready = false;          // Ready to place/spawn
};

// ============================================================
// Superweapon Status
// ============================================================

struct SuperweaponState {
    bool available = false;
    bool charging = false;
    bool ready = false;
    float charge_progress = 0.0f;
    float charge_time = 300.0f;  // 5 minutes default

    void update(float dt) {
        if (charging && !ready) {
            charge_progress += dt / charge_time;
            if (charge_progress >= 1.0f) {
                charge_progress = 1.0f;
                ready = true;
                charging = false;
            }
        }
    }

    void fire() {
        ready = false;
        charge_progress = 0.0f;
        charging = true;
    }
};

// ============================================================
// House (Player)
// ============================================================

class House {
public:
    House();
    ~House() = default;

    void init(HouseType type, const std::string& name, bool is_human);
    void update(float dt, EntityManager& entities);

    // Identity
    HouseType type() const { return type_; }
    const std::string& name() const { return name_; }
    bool is_human() const { return is_human_; }
    bool is_defeated() const { return defeated_; }

    // ---- Economy ----
    int credits() const { return credits_; }
    int capacity() const { return storage_capacity_; }
    void add_credits(int amount);
    bool spend_credits(int amount);
    void set_credits(int amount) { credits_ = amount; }

    // ---- Power ----
    int power_output() const { return power_output_; }
    int power_drain() const { return power_drain_; }
    int power_surplus() const { return power_output_ - power_drain_; }
    bool has_sufficient_power() const { return power_output_ >= power_drain_; }
    float power_ratio() const {
        return power_drain_ > 0 ?
               static_cast<float>(power_output_) / power_drain_ : 1.0f;
    }
    void recalculate_power(const EntityManager& entities);

    // ---- Tech Tree ----
    int tech_level() const { return tech_level_; }
    bool can_build_unit(UnitType type) const;
    bool can_build_infantry(InfantryType type) const;
    bool can_build_structure(BuildingType type) const;
    bool has_building(BuildingType type) const;
    void update_tech_level(const EntityManager& entities);
    std::vector<UnitType> available_units() const;
    std::vector<InfantryType> available_infantry() const;
    std::vector<BuildingType> available_structures() const;

    // ---- Build Queue ----
    bool start_building(ProductionCategory cat, int type_index);
    void cancel_building(ProductionCategory cat);
    void pause_building(ProductionCategory cat);
    void resume_building(ProductionCategory cat);
    const BuildQueueEntry* current_production(ProductionCategory cat) const;
    bool has_ready_production(ProductionCategory cat) const;
    void place_produced(ProductionCategory cat);

    // ---- Superweapons ----
    SuperweaponState& ion_cannon() { return ion_cannon_; }
    SuperweaponState& nuke_strike() { return nuke_strike_; }
    SuperweaponState& airstrike() { return airstrike_; }
    const SuperweaponState& ion_cannon() const { return ion_cannon_; }

    // ---- Unit tracking ----
    void add_unit(EntityID id) { owned_units_.push_back(id); }
    void add_building(EntityID id) { owned_buildings_.push_back(id); }
    void remove_unit(EntityID id);
    void remove_building(EntityID id);
    int unit_count() const { return static_cast<int>(owned_units_.size()); }
    int building_count() const { return static_cast<int>(owned_buildings_.size()); }
    const std::vector<EntityID>& owned_units() const { return owned_units_; }
    const std::vector<EntityID>& owned_buildings() const { return owned_buildings_; }

    // ---- Win/lose ----
    void check_defeat(const EntityManager& entities);
    void set_defeated() { defeated_ = true; }

    // ---- Score ----
    int units_destroyed() const { return units_destroyed_; }
    int buildings_destroyed() const { return buildings_destroyed_; }
    int units_lost() const { return units_lost_; }
    void record_kill() { units_destroyed_++; }
    void record_building_kill() { buildings_destroyed_++; }
    void record_loss() { units_lost_++; }

private:
    void update_production(float dt);
    bool check_prerequisites(const std::vector<BuildingType>& prereqs) const;

    HouseType type_ = HouseType::None;
    std::string name_;
    bool is_human_ = false;
    bool defeated_ = false;

    // Economy
    int credits_ = 0;
    int storage_capacity_ = 2000; // Base + silos
    int tiberium_harvested_ = 0;

    // Power
    int power_output_ = 0;
    int power_drain_ = 0;

    // Tech
    int tech_level_ = 1;
    std::vector<BuildingType> owned_building_types_;

    // Production queues (one per category)
    std::vector<BuildQueueEntry> production_queue_structures_;
    std::vector<BuildQueueEntry> production_queue_units_;
    std::vector<BuildQueueEntry> production_queue_infantry_;

    // Superweapons
    SuperweaponState ion_cannon_;
    SuperweaponState nuke_strike_;
    SuperweaponState airstrike_;

    // Entity tracking
    std::vector<EntityID> owned_units_;
    std::vector<EntityID> owned_buildings_;

    // Score
    int units_destroyed_ = 0;
    int buildings_destroyed_ = 0;
    int units_lost_ = 0;
    int harvester_count_ = 0;
};

// ============================================================
// Game Session
// Manages all houses and the overall game state
// ============================================================

class GameSession {
public:
    GameSession();
    ~GameSession() = default;

    // Setup
    void init(int num_players = 2);
    void setup_skirmish(HouseType player_faction, int num_ai = 1);

    // Update
    void update(float dt);

    // Access
    House& get_house(HouseType type);
    House& player_house() { return houses_[player_house_idx_]; }
    const House& player_house() const { return houses_[player_house_idx_]; }
    EntityManager& entities() { return entities_; }
    TileMap& tilemap() { return *tilemap_; }

    // Game state
    bool is_game_over() const { return game_over_; }
    HouseType winner() const { return winner_; }
    float game_time() const { return game_time_; }

    // Commands (from player input)
    void command_move(const std::vector<EntityID>& units,
                      int target_x, int target_y);
    void command_attack(const std::vector<EntityID>& units,
                        EntityID target);
    void command_harvest(EntityID harvester);
    void command_stop(const std::vector<EntityID>& units);

    // Building placement
    bool can_place_building(BuildingType type, int cell_x, int cell_y) const;
    EntityID place_building(BuildingType type, HouseType owner,
                            int cell_x, int cell_y);

    void set_tilemap(TileMap* map) { tilemap_ = map; }

private:
    void update_combat();
    void update_fog_of_war();
    void check_victory_conditions();

    std::vector<House> houses_;
    int player_house_idx_ = 0;
    EntityManager entities_;
    TileMap* tilemap_ = nullptr;

    bool game_over_ = false;
    HouseType winner_ = HouseType::None;
    float game_time_ = 0.0f;
};

} // namespace CnC
