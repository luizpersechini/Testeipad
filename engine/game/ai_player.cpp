/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * AI Player Implementation
 */

#include "ai_player.h"
#include <algorithm>
#include <cmath>

namespace CnC {

// ============================================================
// AI Build Orders
// ============================================================

static AIBuildOrder get_gdi_build_order() {
    AIBuildOrder bo;
    bo.name = "GDI Standard";
    bo.structures = {
        BuildingType::NUKE,   // Power Plant
        BuildingType::PROC,   // Refinery
        BuildingType::FACT,   // Barracks
        BuildingType::NUKE,   // Second Power
        BuildingType::WEAP,   // Weapons Factory
        BuildingType::GTWR,   // Guard Tower
        BuildingType::PROC,   // Second Refinery
        BuildingType::NUK2,   // Advanced Power
        BuildingType::GTWR,   // Second Guard Tower
        BuildingType::REPAIR, // Repair Bay
        BuildingType::ATWR,   // Advanced Guard Tower
        BuildingType::HPAD,   // Helipad
    };
    bo.initial_units = {
        UnitType::JEEP, UnitType::MTANK, UnitType::MTANK,
        UnitType::APC, UnitType::MTANK
    };
    bo.initial_infantry = {
        InfantryType::E1, InfantryType::E1, InfantryType::E3,
        InfantryType::E1, InfantryType::E3
    };
    return bo;
}

static AIBuildOrder get_nod_build_order() {
    AIBuildOrder bo;
    bo.name = "Nod Rush";
    bo.structures = {
        BuildingType::NUKE,   // Power Plant
        BuildingType::PROC,   // Refinery
        BuildingType::HAND,   // Hand of Nod
        BuildingType::NUKE,   // Second Power
        BuildingType::AFLD,   // Airstrip
        BuildingType::GUN,    // Turret
        BuildingType::PROC,   // Second Refinery
        BuildingType::GUN,    // Second Turret
        BuildingType::NUK2,   // Advanced Power
        BuildingType::OBELISK,// Obelisk of Light
        BuildingType::SAM,    // SAM Site
        BuildingType::TMPL,   // Temple of Nod
    };
    bo.initial_units = {
        UnitType::BUGGY, UnitType::BIKE, UnitType::LTANK,
        UnitType::LTANK, UnitType::ARTY
    };
    bo.initial_infantry = {
        InfantryType::E1, InfantryType::E1, InfantryType::E4,
        InfantryType::E1, InfantryType::E3
    };
    return bo;
}

// ============================================================
// AIPlayer
// ============================================================

AIPlayer::AIPlayer()
    : rng_(std::random_device{}()) {
}

void AIPlayer::init(HouseType house, AIDifficulty difficulty) {
    house_ = house;
    difficulty_ = difficulty;
    state_ = AIState::BuildingBase;

    // Set build order based on faction
    if (house == HouseType::GDI || house == HouseType::Multi1 ||
        house == HouseType::Multi3 || house == HouseType::Multi5) {
        build_order_ = get_gdi_build_order();
    } else {
        build_order_ = get_nod_build_order();
    }

    // Adjust parameters by difficulty
    switch (difficulty) {
        case AIDifficulty::Easy:
            think_interval_ = 4.0f;
            build_check_interval_ = 10.0f;
            attack_check_interval_ = 60.0f;
            attack_threshold_ = 8;
            attack_cooldown_max_ = 120.0f;
            scout_interval_ = 40.0f;
            break;

        case AIDifficulty::Normal:
            think_interval_ = 2.0f;
            build_check_interval_ = 5.0f;
            attack_check_interval_ = 30.0f;
            attack_threshold_ = 5;
            attack_cooldown_max_ = 60.0f;
            scout_interval_ = 20.0f;
            break;

        case AIDifficulty::Hard:
            think_interval_ = 1.0f;
            build_check_interval_ = 3.0f;
            attack_check_interval_ = 15.0f;
            attack_threshold_ = 3;
            attack_cooldown_max_ = 30.0f;
            scout_interval_ = 10.0f;
            break;
    }
}

void AIPlayer::update(float dt, GameSession& session, TileMap& tilemap) {
    think_timer_ += dt;
    build_timer_ += dt;
    attack_timer_ += dt;
    scout_timer_ += dt;
    attack_cooldown_ -= dt;

    if (attack_cooldown_ < 0.0f) attack_cooldown_ = 0.0f;

    // Periodic state evaluation
    if (think_timer_ >= think_interval_) {
        think_timer_ = 0.0f;
        evaluate_state(session, tilemap);
    }

    // Execute current state behaviors
    execute_state(dt, session, tilemap);
}

void AIPlayer::evaluate_state(GameSession& session, TileMap& tilemap) {
    House& house = session.get_house(house_);

    // Check if defeated
    if (house.is_defeated()) return;

    // Check if base is under attack
    if (is_base_threatened(session)) {
        if (state_ != AIState::Defending) {
            state_ = AIState::Defending;
            return;
        }
    }

    // State transitions based on game progress
    int num_buildings = house.building_count();
    int num_military = total_military_units(session);
    int build_steps_done = build_order_step_;
    int total_build_steps = static_cast<int>(build_order_.structures.size());

    // Still in initial build order?
    if (build_steps_done < total_build_steps / 2) {
        state_ = AIState::BuildingBase;
        return;
    }

    // Economy check
    if (need_more_harvesters(session) || need_more_refineries(house)) {
        state_ = AIState::Expanding;
        return;
    }

    // Power check
    if (need_more_power(house)) {
        state_ = AIState::BuildingBase;
        return;
    }

    // Have enough army to attack?
    if (num_military >= attack_threshold_ && attack_cooldown_ <= 0.0f) {
        state_ = AIState::Attacking;
        return;
    }

    // Continue building army
    if (build_steps_done >= total_build_steps / 2) {
        state_ = AIState::Massing;
        return;
    }

    // Default: keep building base
    state_ = AIState::BuildingBase;
}

void AIPlayer::execute_state(float dt, GameSession& session, TileMap& tilemap) {
    // Always do building and production
    if (build_timer_ >= build_check_interval_) {
        build_timer_ = 0.0f;
        think_building(dt, session);
        think_production(dt, session);
        think_economy(dt, session);
    }

    // Always scout
    if (scout_timer_ >= scout_interval_) {
        scout_timer_ = 0.0f;
        think_scouting(dt, session, tilemap);
    }

    // State-specific behavior
    switch (state_) {
        case AIState::BuildingBase:
        case AIState::Expanding:
        case AIState::Teching:
            // Passive - just build
            break;

        case AIState::Massing:
            think_military(dt, session, tilemap);
            break;

        case AIState::Attacking:
            if (attack_timer_ >= attack_check_interval_) {
                attack_timer_ = 0.0f;
                launch_attack(session, tilemap);
            }
            break;

        case AIState::Defending:
            defend_base(session, tilemap);
            break;

        case AIState::Harassing:
            // Send light units to harass
            think_military(dt, session, tilemap);
            break;

        case AIState::Retreating:
            // Pull damaged units back to base
            state_ = AIState::Massing;
            break;
    }
}

// ============================================================
// Building Logic
// ============================================================

void AIPlayer::think_building(float dt, GameSession& session) {
    House& house = session.get_house(house_);

    // Don't build if no construction yard
    if (!house.has_building(BuildingType::HQ)) return;

    // Check if already building a structure
    const BuildQueueEntry* current = house.current_production(ProductionCategory::Structure);
    if (current && !current->ready) return;

    // If a structure is ready, place it
    if (current && current->ready) {
        // Find a location near base
        BuildingType type = static_cast<BuildingType>(current->type_index);
        int loc = find_build_location(type, session, session.tilemap());
        if (loc >= 0) {
            int cx = loc % MAP_MAX_WIDTH;
            int cy = loc / MAP_MAX_WIDTH;
            if (session.can_place_building(type, cx, cy)) {
                session.place_building(type, house_, cx, cy);
                house.place_produced(ProductionCategory::Structure);
                build_order_step_++;
            }
        }
        return;
    }

    // Determine what to build next
    BuildingType next = next_building_to_construct(house);
    if (next != BuildingType::None) {
        house.start_building(ProductionCategory::Structure,
                            static_cast<int>(next));
    }
}

BuildingType AIPlayer::next_building_to_construct(const House& house) const {
    // Priority overrides
    if (need_more_power(house)) {
        if (house.can_build_structure(BuildingType::NUK2))
            return BuildingType::NUK2;
        return BuildingType::NUKE;
    }

    if (need_more_refineries(house)) {
        return BuildingType::PROC;
    }

    // Follow build order
    if (build_order_step_ < static_cast<int>(build_order_.structures.size())) {
        BuildingType next = build_order_.structures[build_order_step_];
        if (house.can_build_structure(next)) {
            return next;
        }
        // If can't build current step, skip it
        // (prerequisites not met yet)
    }

    return BuildingType::None;
}

int AIPlayer::find_build_location(BuildingType type, const GameSession& session,
                                   const TileMap& tilemap) const {
    const auto& data = GameDataRegistry::instance().get_building(type);
    int w = data.width_cells;
    int h = data.height_cells;

    // Spiral outward from base location looking for valid placement
    for (int radius = 1; radius < 15; radius++) {
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (std::abs(dx) != radius && std::abs(dy) != radius) continue;

                int cx = base_x_ + dx;
                int cy = base_y_ + dy;

                if (session.can_place_building(type, cx, cy)) {
                    return cy * MAP_MAX_WIDTH + cx;
                }
            }
        }
    }

    return -1; // No valid location found
}

// ============================================================
// Production Logic
// ============================================================

void AIPlayer::think_production(float dt, GameSession& session) {
    House& house = session.get_house(house_);

    // Unit production
    if (house.has_building(BuildingType::WEAP) ||
        house.has_building(BuildingType::AFLD)) {

        const BuildQueueEntry* unit_prod = house.current_production(ProductionCategory::Unit);

        if (!unit_prod) {
            // Decide what unit to build
            UnitType to_build = UnitType::None;

            // Priority: harvesters
            if (need_more_harvesters(session)) {
                to_build = UnitType::HARV;
            } else {
                // Build from initial unit list, then cycle through combat units
                int military = total_military_units(session);
                int idx = military % static_cast<int>(build_order_.initial_units.size());
                to_build = build_order_.initial_units[idx];
            }

            if (to_build != UnitType::None && house.can_build_unit(to_build)) {
                house.start_building(ProductionCategory::Unit,
                                    static_cast<int>(to_build));
            }
        } else if (unit_prod->ready) {
            // Unit is ready - deploy it
            house.place_produced(ProductionCategory::Unit);
        }
    }

    // Infantry production
    if (house.has_building(BuildingType::HAND) ||
        house.has_building(BuildingType::FACT)) {

        const BuildQueueEntry* inf_prod = house.current_production(ProductionCategory::Infantry);

        if (!inf_prod) {
            int inf_count = 0;
            for (const auto& inf : session.entities().all_infantry()) {
                if (inf.owner == house_ && inf.is_alive()) inf_count++;
            }

            if (inf_count < 15) {  // Cap on infantry
                int idx = inf_count % static_cast<int>(build_order_.initial_infantry.size());
                InfantryType to_build = build_order_.initial_infantry[idx];

                if (house.can_build_infantry(to_build)) {
                    house.start_building(ProductionCategory::Infantry,
                                        static_cast<int>(to_build));
                }
            }
        } else if (inf_prod->ready) {
            house.place_produced(ProductionCategory::Infantry);
        }
    }
}

// ============================================================
// Economy Logic
// ============================================================

void AIPlayer::think_economy(float dt, GameSession& session) {
    // Handled through production priorities (harvesters, refineries)
}

bool AIPlayer::need_more_harvesters(const GameSession& session) const {
    int harvester_count = count_units_of_type(UnitType::HARV, session);
    int refinery_count = count_buildings_of_type(BuildingType::PROC, session);
    // At least 1 harvester per refinery
    return harvester_count < refinery_count;
}

bool AIPlayer::need_more_power(const House& house) const {
    return house.power_surplus() < 0;
}

bool AIPlayer::need_more_refineries(const House& house) const {
    // Build a second refinery if we can afford it and only have one
    int refinery_count = 0;
    for (const auto& bt : house.available_structures()) {
        // Count existing refineries through building types
        if (bt == BuildingType::PROC) { /* available to build */ }
    }
    // Simple heuristic: if credits > 3000 and only 1 refinery, build another
    return house.credits() > 3000 && house.building_count() > 3;
}

// ============================================================
// Military Logic
// ============================================================

void AIPlayer::think_military(float dt, GameSession& session, TileMap& tilemap) {
    // If we have enough units, assemble an attack group
    int military = total_military_units(session);
    if (military >= attack_threshold_) {
        assemble_attack_force(session);
    }
}

void AIPlayer::assemble_attack_force(GameSession& session) {
    House& house = session.get_house(house_);
    auto& entities = session.entities();

    AIAttackGroup group;

    // Gather idle military units
    for (EntityID uid : house.owned_units()) {
        Unit* unit = entities.get_unit(uid);
        if (!unit || !unit->is_alive()) continue;
        if (unit->type == UnitType::HARV || unit->type == UnitType::MCV) continue;

        // Only grab units in guard or sleep mode (not already moving/attacking)
        if (unit->mission == MissionType::Guard ||
            unit->mission == MissionType::Sleep) {
            group.units.push_back(uid);
        }
    }

    if (static_cast<int>(group.units.size()) >= attack_threshold_) {
        // Set rally point near base
        group.target_x = base_x_;
        group.target_y = base_y_;
        group.assembled = true;
        attack_groups_.push_back(group);
    }
}

void AIPlayer::launch_attack(GameSession& session, TileMap& tilemap) {
    if (attack_cooldown_ > 0.0f) return;

    // Find target
    Point target = find_attack_target(session, tilemap);
    if (target.x < 0 || target.y < 0) return;

    auto& entities = session.entities();

    // Send all assembled groups
    for (auto& group : attack_groups_) {
        if (!group.assembled) continue;

        std::vector<EntityID> alive_units;
        for (EntityID uid : group.units) {
            Unit* unit = entities.get_unit(uid);
            if (unit && unit->is_alive()) {
                alive_units.push_back(uid);
            }
        }

        if (!alive_units.empty()) {
            // Convert cell target to pixel coordinates for move command
            int px = target.x * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2;
            int py = target.y * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2;
            session.command_move(alive_units, px, py);
            group.attacking = true;
        }
    }

    // Clean up empty groups
    attack_groups_.erase(
        std::remove_if(attack_groups_.begin(), attack_groups_.end(),
            [](const AIAttackGroup& g) { return g.units.empty(); }),
        attack_groups_.end()
    );

    attack_cooldown_ = attack_cooldown_max_;
    state_ = AIState::Massing;  // Start rebuilding
}

void AIPlayer::defend_base(GameSession& session, TileMap& tilemap) {
    House& house = session.get_house(house_);
    auto& entities = session.entities();

    // Find threatening enemy units near base
    Rect base_area(
        (base_x_ - 10) * CELL_PIXEL_WIDTH,
        (base_y_ - 10) * CELL_PIXEL_HEIGHT,
        20 * CELL_PIXEL_WIDTH,
        20 * CELL_PIXEL_HEIGHT
    );

    // Find closest enemy
    EntityID closest_threat = INVALID_ENTITY;
    float closest_dist = 999999.0f;

    for (const auto& unit : entities.all_units()) {
        if (unit.owner == house_ || !unit.is_alive()) continue;

        float dx = unit.x - base_x_ * CELL_PIXEL_WIDTH;
        float dy = unit.y - base_y_ * CELL_PIXEL_HEIGHT;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < 15 * CELL_PIXEL_WIDTH && dist < closest_dist) {
            closest_dist = dist;
            closest_threat = unit.id;
        }
    }

    if (closest_threat != INVALID_ENTITY) {
        // Rally all military units to attack the threat
        std::vector<EntityID> defenders;
        for (EntityID uid : house.owned_units()) {
            Unit* unit = entities.get_unit(uid);
            if (!unit || !unit->is_alive()) continue;
            if (unit->type == UnitType::HARV || unit->type == UnitType::MCV) continue;
            defenders.push_back(uid);
        }

        if (!defenders.empty()) {
            session.command_attack(defenders, closest_threat);
        }
    } else {
        // Threat gone, return to previous state
        state_ = AIState::Massing;
    }
}

Point AIPlayer::find_attack_target(const GameSession& session,
                                    const TileMap& tilemap) const {
    auto& entities = session.entities();

    // If we know where enemy base is, attack there
    if (enemy_base_x_ >= 0 && enemy_base_y_ >= 0) {
        return Point(enemy_base_x_, enemy_base_y_);
    }

    // Look for enemy buildings
    for (const auto& bld : entities.all_buildings()) {
        if (bld.owner != house_ && bld.is_alive()) {
            return Point(bld.cell_x(), bld.cell_y());
        }
    }

    // Look for enemy units
    for (const auto& unit : entities.all_units()) {
        if (unit.owner != house_ && unit.is_alive()) {
            return Point(unit.cell_x(), unit.cell_y());
        }
    }

    return Point(-1, -1); // No target found
}

bool AIPlayer::is_base_threatened(const GameSession& session) const {
    auto& entities = session.entities();

    // Check for enemy units near base
    float threat_range = 12.0f * CELL_PIXEL_WIDTH;
    float base_px = static_cast<float>(base_x_ * CELL_PIXEL_WIDTH);
    float base_py = static_cast<float>(base_y_ * CELL_PIXEL_HEIGHT);

    for (const auto& unit : entities.all_units()) {
        if (unit.owner == house_ || !unit.is_alive()) continue;

        float dx = unit.x - base_px;
        float dy = unit.y - base_py;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < threat_range) return true;
    }

    for (const auto& inf : entities.all_infantry()) {
        if (inf.owner == house_ || !inf.is_alive()) continue;

        float dx = inf.x - base_px;
        float dy = inf.y - base_py;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < threat_range) return true;
    }

    return false;
}

// ============================================================
// Scouting
// ============================================================

void AIPlayer::think_scouting(float dt, GameSession& session, TileMap& tilemap) {
    if (has_found_enemy_) return; // Already know where enemy is

    auto& entities = session.entities();

    // Check if we can see any enemy buildings
    for (const auto& bld : entities.all_buildings()) {
        if (bld.owner != house_ && bld.owner != HouseType::Neutral) {
            enemy_base_x_ = bld.cell_x();
            enemy_base_y_ = bld.cell_y();
            has_found_enemy_ = true;
            return;
        }
    }

    // Send a scout if we have one
    House& house = session.get_house(house_);
    if (scout_unit_ == INVALID_ENTITY) {
        // Find a fast unit to scout with
        for (EntityID uid : house.owned_units()) {
            Unit* unit = entities.get_unit(uid);
            if (!unit || !unit->is_alive()) continue;
            if (unit->type == UnitType::JEEP || unit->type == UnitType::BUGGY ||
                unit->type == UnitType::BIKE) {
                scout_unit_ = uid;
                break;
            }
        }
    }

    if (scout_unit_ != INVALID_ENTITY) {
        Unit* scout = entities.get_unit(scout_unit_);
        if (scout && scout->is_alive() && !scout->is_moving) {
            Point target = pick_scout_target(tilemap);
            int px = target.x * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2;
            int py = target.y * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2;
            session.command_move({scout_unit_}, px, py);
        } else if (!scout || !scout->is_alive()) {
            scout_unit_ = INVALID_ENTITY; // Scout lost
        }
    }
}

Point AIPlayer::pick_scout_target(const TileMap& tilemap) const {
    // Scout unexplored areas, biased toward opposite corner from base
    int map_w = tilemap.width();
    int map_h = tilemap.height();

    // Opposite corner from base
    int target_x = (base_x_ < map_w / 2) ? map_w - 6 : 6;
    int target_y = (base_y_ < map_h / 2) ? map_h - 6 : 6;

    return Point(target_x, target_y);
}

// ============================================================
// Utility
// ============================================================

int AIPlayer::count_units_of_type(UnitType type,
                                   const GameSession& session) const {
    int count = 0;
    for (const auto& unit : session.entities().all_units()) {
        if (unit.owner == house_ && unit.type == type && unit.is_alive())
            count++;
    }
    return count;
}

int AIPlayer::count_buildings_of_type(BuildingType type,
                                       const GameSession& session) const {
    int count = 0;
    for (const auto& bld : session.entities().all_buildings()) {
        if (bld.owner == house_ && bld.type == type && bld.is_alive())
            count++;
    }
    return count;
}

int AIPlayer::total_military_units(const GameSession& session) const {
    int count = 0;
    for (const auto& unit : session.entities().all_units()) {
        if (unit.owner == house_ && unit.is_alive() &&
            unit.type != UnitType::HARV && unit.type != UnitType::MCV)
            count++;
    }
    for (const auto& inf : session.entities().all_infantry()) {
        if (inf.owner == house_ && inf.is_alive())
            count++;
    }
    return count;
}

float AIPlayer::army_strength(const GameSession& session) const {
    float strength = 0.0f;
    auto& data = GameDataRegistry::instance();

    for (const auto& unit : session.entities().all_units()) {
        if (unit.owner == house_ && unit.is_alive() && unit.type_data) {
            strength += static_cast<float>(unit.type_data->cost) *
                        unit.health_ratio();
        }
    }
    for (const auto& inf : session.entities().all_infantry()) {
        if (inf.owner == house_ && inf.is_alive() && inf.type_data) {
            strength += static_cast<float>(inf.type_data->cost) *
                        inf.health_ratio();
        }
    }

    return strength;
}

// ============================================================
// AIManager
// ============================================================

void AIManager::add_ai(HouseType house, AIDifficulty difficulty,
                       int base_x, int base_y) {
    AIPlayer ai;
    ai.init(house, difficulty);
    ai.set_base_location(base_x, base_y);
    ai_players_.push_back(ai);
}

void AIManager::update(float dt, GameSession& session, TileMap& tilemap) {
    for (auto& ai : ai_players_) {
        ai.update(dt, session, tilemap);
    }
}

} // namespace CnC
