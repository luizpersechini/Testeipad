/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * AI Player System
 *
 * Implements a basic AI opponent that can:
 *   - Build a base following a build order
 *   - Train units and manage economy
 *   - Scout the map
 *   - Launch attacks against the player
 *   - Defend its base when under attack
 *
 * The AI uses a simple state machine with priorities rather than
 * the complex trigger-based system of the original. This provides
 * believable behavior while remaining maintainable.
 */

#pragma once

#include "game_types.h"
#include "unit.h"
#include "house.h"
#include "../graphics/tilemap.h"
#include <vector>
#include <random>

namespace CnC {

// Forward declaration
class GameSession;

// ============================================================
// AI Difficulty
// ============================================================

enum class AIDifficulty {
    Easy,      // Slow build, small attacks, poor targeting
    Normal,    // Balanced
    Hard       // Fast build, large coordinated attacks
};

// ============================================================
// AI State Machine
// ============================================================

enum class AIState {
    BuildingBase,     // Constructing initial structures
    Expanding,        // Building economy (harvesters, refineries)
    Teching,          // Upgrading tech tree
    Massing,          // Building up army
    Attacking,        // Launching assault
    Defending,        // Under attack, pulling back
    Harassing,        // Hit-and-run with light units
    Retreating        // Pulling damaged units back
};

// ============================================================
// AI Build Order
// Sequence of structures to construct
// ============================================================

struct AIBuildOrder {
    std::string name;
    std::vector<BuildingType> structures;
    std::vector<UnitType> initial_units;     // First units to build
    std::vector<InfantryType> initial_infantry;
};

// ============================================================
// AI Attack Group
// ============================================================

struct AIAttackGroup {
    std::vector<EntityID> units;
    int target_x = 0;
    int target_y = 0;
    bool assembled = false;    // All units gathered at rally point
    bool attacking = false;
    float idle_timer = 0.0f;   // Time spent idle (triggers new orders)
};

// ============================================================
// AI Player
// ============================================================

class AIPlayer {
public:
    AIPlayer();
    ~AIPlayer() = default;

    void init(HouseType house, AIDifficulty difficulty);
    void update(float dt, GameSession& session, TileMap& tilemap);

    // Configuration
    HouseType house() const { return house_; }
    AIDifficulty difficulty() const { return difficulty_; }
    AIState current_state() const { return state_; }

    void set_base_location(int cell_x, int cell_y) {
        base_x_ = cell_x;
        base_y_ = cell_y;
    }

private:
    // Decision making
    void evaluate_state(GameSession& session, TileMap& tilemap);
    void execute_state(float dt, GameSession& session, TileMap& tilemap);

    // Building
    void think_building(float dt, GameSession& session);
    void think_production(float dt, GameSession& session);
    BuildingType next_building_to_construct(const House& house) const;
    int find_build_location(BuildingType type, const GameSession& session,
                            const TileMap& tilemap) const;

    // Economy
    void think_economy(float dt, GameSession& session);
    bool need_more_harvesters(const GameSession& session) const;
    bool need_more_power(const House& house) const;
    bool need_more_refineries(const House& house) const;

    // Military
    void think_military(float dt, GameSession& session, TileMap& tilemap);
    void assemble_attack_force(GameSession& session);
    void launch_attack(GameSession& session, TileMap& tilemap);
    void defend_base(GameSession& session, TileMap& tilemap);
    Point find_attack_target(const GameSession& session,
                             const TileMap& tilemap) const;
    bool is_base_threatened(const GameSession& session) const;

    // Scouting
    void think_scouting(float dt, GameSession& session, TileMap& tilemap);
    Point pick_scout_target(const TileMap& tilemap) const;

    // Utilities
    int count_units_of_type(UnitType type, const GameSession& session) const;
    int count_buildings_of_type(BuildingType type,
                                const GameSession& session) const;
    int total_military_units(const GameSession& session) const;
    float army_strength(const GameSession& session) const;

    // Identity
    HouseType house_ = HouseType::Nod;
    AIDifficulty difficulty_ = AIDifficulty::Normal;
    AIState state_ = AIState::BuildingBase;

    // Base location
    int base_x_ = 0;
    int base_y_ = 0;

    // Build order tracking
    AIBuildOrder build_order_;
    int build_order_step_ = 0;

    // Timing
    float think_timer_ = 0.0f;
    float build_timer_ = 0.0f;
    float attack_timer_ = 0.0f;
    float scout_timer_ = 0.0f;

    // Think intervals (seconds) - adjusted by difficulty
    float think_interval_ = 2.0f;
    float build_check_interval_ = 5.0f;
    float attack_check_interval_ = 30.0f;
    float scout_interval_ = 20.0f;

    // Attack management
    std::vector<AIAttackGroup> attack_groups_;
    int attack_threshold_ = 5;  // Min units before attacking
    float attack_cooldown_ = 0.0f;
    float attack_cooldown_max_ = 60.0f;

    // Scouting
    EntityID scout_unit_ = INVALID_ENTITY;
    bool has_found_enemy_ = false;
    int enemy_base_x_ = -1;
    int enemy_base_y_ = -1;

    // Random number generator
    std::mt19937 rng_;
};

// ============================================================
// AI Manager
// Manages all AI players in a game session
// ============================================================

class AIManager {
public:
    AIManager() = default;

    void add_ai(HouseType house, AIDifficulty difficulty,
                int base_x, int base_y);
    void update(float dt, GameSession& session, TileMap& tilemap);
    void clear() { ai_players_.clear(); }

    int count() const { return static_cast<int>(ai_players_.size()); }

private:
    std::vector<AIPlayer> ai_players_;
};

} // namespace CnC
