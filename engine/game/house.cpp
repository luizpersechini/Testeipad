/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * House & Game Session Implementation
 */

#include "house.h"
#include <algorithm>
#include <cmath>

namespace CnC {

// ============================================================
// House
// ============================================================

House::House() = default;

void House::init(HouseType type, const std::string& name, bool is_human) {
    type_ = type;
    name_ = name;
    is_human_ = is_human;
    credits_ = 0;
    tech_level_ = 1;
    defeated_ = false;
}

void House::update(float dt, EntityManager& entities) {
    update_production(dt);
    recalculate_power(entities);
    update_tech_level(entities);

    // Update superweapons
    ion_cannon_.update(dt);
    nuke_strike_.update(dt);
    airstrike_.update(dt);

    // Check for Ion Cannon availability (GDI with Adv Comm Center)
    if (type_ == HouseType::GDI && has_building(BuildingType::EYE)) {
        if (!ion_cannon_.available) {
            ion_cannon_.available = true;
            ion_cannon_.charging = true;
        }
    }

    // Check for Nuke availability (Nod with Temple)
    if (type_ == HouseType::Nod && has_building(BuildingType::TMPL)) {
        if (!nuke_strike_.available) {
            nuke_strike_.available = true;
            nuke_strike_.charging = true;
        }
    }
}

// ---- Economy ----

void House::add_credits(int amount) {
    credits_ += amount;
    if (credits_ > storage_capacity_) {
        credits_ = storage_capacity_; // Can't store more than capacity
    }
    tiberium_harvested_ += amount;
}

bool House::spend_credits(int amount) {
    if (credits_ >= amount) {
        credits_ -= amount;
        return true;
    }
    return false;
}

// ---- Power ----

void House::recalculate_power(const EntityManager& entities) {
    power_output_ = 0;
    power_drain_ = 0;
    storage_capacity_ = 2000; // Base capacity

    for (EntityID bid : owned_buildings_) {
        const Building* bld = const_cast<EntityManager&>(entities).get_building(bid);
        if (!bld || !bld->is_alive() || bld->under_construction) continue;

        int power = bld->power_output;
        if (power > 0) {
            power_output_ += power;
        } else {
            power_drain_ += -power;
        }

        // Silos add storage
        if (bld->type == BuildingType::SILO) {
            storage_capacity_ += 1500;
        }
        if (bld->type == BuildingType::PROC) {
            storage_capacity_ += 1000;
        }
    }
}

// ---- Tech Tree ----

void House::update_tech_level(const EntityManager& entities) {
    owned_building_types_.clear();

    for (EntityID bid : owned_buildings_) {
        const Building* bld = const_cast<EntityManager&>(entities).get_building(bid);
        if (!bld || !bld->is_alive() || bld->under_construction) continue;

        // Track which building types we have
        if (std::find(owned_building_types_.begin(),
                      owned_building_types_.end(),
                      bld->type) == owned_building_types_.end()) {
            owned_building_types_.push_back(bld->type);
        }
    }

    // Tech level is derived from the highest-tech building we have
    tech_level_ = 1;
    const auto& registry = GameDataRegistry::instance();
    for (BuildingType bt : owned_building_types_) {
        int tl = registry.get_building(bt).tech_level;
        if (tl > tech_level_) tech_level_ = tl;
    }
}

bool House::has_building(BuildingType type) const {
    return std::find(owned_building_types_.begin(),
                     owned_building_types_.end(),
                     type) != owned_building_types_.end();
}

bool House::check_prerequisites(const std::vector<BuildingType>& prereqs) const {
    for (BuildingType bt : prereqs) {
        if (!has_building(bt)) return false;
    }
    return true;
}

bool House::can_build_unit(UnitType type) const {
    const auto& data = GameDataRegistry::instance().get_unit(type);
    if (data.tech_level > tech_level_) return false;
    if (data.owner != HouseType::None && data.owner != type_) return false;
    if (!check_prerequisites(data.prerequisites)) return false;
    if (!has_building(BuildingType::WEAP)) return false;
    return true;
}

bool House::can_build_infantry(InfantryType type) const {
    const auto& data = GameDataRegistry::instance().get_infantry(type);
    if (data.tech_level > tech_level_) return false;
    if (data.owner != HouseType::None && data.owner != type_) return false;
    if (!check_prerequisites(data.prerequisites)) return false;

    // Need barracks (GDI) or Hand of Nod (Nod)
    if (type_ == HouseType::GDI && !has_building(BuildingType::FACT)) return false;
    if (type_ == HouseType::Nod && !has_building(BuildingType::HAND)) return false;
    return true;
}

bool House::can_build_structure(BuildingType type) const {
    const auto& data = GameDataRegistry::instance().get_building(type);
    if (data.tech_level > tech_level_) return false;
    if (data.owner != HouseType::None && data.owner != type_) return false;
    if (!check_prerequisites(data.prerequisites)) return false;
    if (!has_building(BuildingType::HQ)) return false;
    return true;
}

std::vector<UnitType> House::available_units() const {
    std::vector<UnitType> result;
    for (int i = 0; i < static_cast<int>(UnitType::Count); i++) {
        UnitType ut = static_cast<UnitType>(i);
        if (can_build_unit(ut)) result.push_back(ut);
    }
    return result;
}

std::vector<InfantryType> House::available_infantry() const {
    std::vector<InfantryType> result;
    for (int i = 0; i < static_cast<int>(InfantryType::Count); i++) {
        InfantryType it = static_cast<InfantryType>(i);
        if (can_build_infantry(it)) result.push_back(it);
    }
    return result;
}

std::vector<BuildingType> House::available_structures() const {
    std::vector<BuildingType> result;
    for (int i = 0; i < static_cast<int>(BuildingType::Count); i++) {
        BuildingType bt = static_cast<BuildingType>(i);
        if (can_build_structure(bt)) result.push_back(bt);
    }
    return result;
}

// ---- Build Queue ----

bool House::start_building(ProductionCategory cat, int type_index) {
    std::vector<BuildQueueEntry>* queue = nullptr;
    int cost = 0;
    float build_time = 0;

    switch (cat) {
        case ProductionCategory::Structure: {
            queue = &production_queue_structures_;
            auto bt = static_cast<BuildingType>(type_index);
            if (!can_build_structure(bt)) return false;
            const auto& data = GameDataRegistry::instance().get_building(bt);
            cost = data.cost;
            build_time = data.build_time / 15.0f; // Convert ticks to seconds
            break;
        }
        case ProductionCategory::Unit: {
            queue = &production_queue_units_;
            auto ut = static_cast<UnitType>(type_index);
            if (!can_build_unit(ut)) return false;
            const auto& data = GameDataRegistry::instance().get_unit(ut);
            cost = data.cost;
            build_time = data.build_time / 15.0f;
            break;
        }
        case ProductionCategory::Infantry: {
            queue = &production_queue_infantry_;
            auto it = static_cast<InfantryType>(type_index);
            if (!can_build_infantry(it)) return false;
            const auto& data = GameDataRegistry::instance().get_infantry(it);
            cost = data.cost;
            build_time = data.build_time / 15.0f;
            break;
        }
    }

    if (!queue) return false;
    if (!spend_credits(cost)) return false;

    BuildQueueEntry entry;
    entry.category = cat;
    entry.type_index = type_index;
    entry.progress = 0.0f;
    entry.build_time = std::max(1.0f, build_time);
    entry.cost = cost;
    entry.paused = false;
    entry.ready = false;

    queue->push_back(entry);
    return true;
}

void House::cancel_building(ProductionCategory cat) {
    std::vector<BuildQueueEntry>* queue = nullptr;
    switch (cat) {
        case ProductionCategory::Structure: queue = &production_queue_structures_; break;
        case ProductionCategory::Unit: queue = &production_queue_units_; break;
        case ProductionCategory::Infantry: queue = &production_queue_infantry_; break;
    }
    if (queue && !queue->empty()) {
        // Refund half the cost
        add_credits(queue->front().cost / 2);
        queue->erase(queue->begin());
    }
}

void House::pause_building(ProductionCategory cat) {
    std::vector<BuildQueueEntry>* queue = nullptr;
    switch (cat) {
        case ProductionCategory::Structure: queue = &production_queue_structures_; break;
        case ProductionCategory::Unit: queue = &production_queue_units_; break;
        case ProductionCategory::Infantry: queue = &production_queue_infantry_; break;
    }
    if (queue && !queue->empty()) {
        queue->front().paused = true;
    }
}

void House::resume_building(ProductionCategory cat) {
    std::vector<BuildQueueEntry>* queue = nullptr;
    switch (cat) {
        case ProductionCategory::Structure: queue = &production_queue_structures_; break;
        case ProductionCategory::Unit: queue = &production_queue_units_; break;
        case ProductionCategory::Infantry: queue = &production_queue_infantry_; break;
    }
    if (queue && !queue->empty()) {
        queue->front().paused = false;
    }
}

const BuildQueueEntry* House::current_production(ProductionCategory cat) const {
    const std::vector<BuildQueueEntry>* queue = nullptr;
    switch (cat) {
        case ProductionCategory::Structure: queue = &production_queue_structures_; break;
        case ProductionCategory::Unit: queue = &production_queue_units_; break;
        case ProductionCategory::Infantry: queue = &production_queue_infantry_; break;
    }
    if (queue && !queue->empty()) return &queue->front();
    return nullptr;
}

bool House::has_ready_production(ProductionCategory cat) const {
    const auto* entry = current_production(cat);
    return entry && entry->ready;
}

void House::place_produced(ProductionCategory cat) {
    std::vector<BuildQueueEntry>* queue = nullptr;
    switch (cat) {
        case ProductionCategory::Structure: queue = &production_queue_structures_; break;
        case ProductionCategory::Unit: queue = &production_queue_units_; break;
        case ProductionCategory::Infantry: queue = &production_queue_infantry_; break;
    }
    if (queue && !queue->empty() && queue->front().ready) {
        queue->erase(queue->begin());
    }
}

void House::update_production(float dt) {
    // Power affects build speed
    float power_factor = has_sufficient_power() ? 1.0f :
                          std::max(0.3f, power_ratio());

    auto update_queue = [&](std::vector<BuildQueueEntry>& queue) {
        if (queue.empty()) return;
        auto& entry = queue.front();
        if (entry.paused || entry.ready) return;

        float rate = (1.0f / entry.build_time) * power_factor;
        entry.progress += rate * dt;

        if (entry.progress >= 1.0f) {
            entry.progress = 1.0f;
            entry.ready = true;
        }
    };

    update_queue(production_queue_structures_);
    update_queue(production_queue_units_);
    update_queue(production_queue_infantry_);
}

// ---- Unit tracking ----

void House::remove_unit(EntityID id) {
    owned_units_.erase(
        std::remove(owned_units_.begin(), owned_units_.end(), id),
        owned_units_.end());
}

void House::remove_building(EntityID id) {
    owned_buildings_.erase(
        std::remove(owned_buildings_.begin(), owned_buildings_.end(), id),
        owned_buildings_.end());
}

// ---- Victory ----

void House::check_defeat(const EntityManager& entities) {
    if (defeated_) return;

    // Defeated if no buildings and no MCV
    bool has_buildings = false;
    for (EntityID bid : owned_buildings_) {
        const Building* bld = const_cast<EntityManager&>(entities).get_building(bid);
        if (bld && bld->is_alive()) {
            has_buildings = true;
            break;
        }
    }

    bool has_mcv = false;
    for (EntityID uid : owned_units_) {
        const Unit* u = const_cast<EntityManager&>(entities).get_unit(uid);
        if (u && u->is_alive() && u->type == UnitType::MCV) {
            has_mcv = true;
            break;
        }
    }

    if (!has_buildings && !has_mcv) {
        defeated_ = true;
    }
}

// ============================================================
// Game Session
// ============================================================

GameSession::GameSession() = default;

void GameSession::init(int num_players) {
    houses_.resize(num_players);
    game_over_ = false;
    winner_ = HouseType::None;
    game_time_ = 0.0f;
}

void GameSession::setup_skirmish(HouseType player_faction, int num_ai) {
    init(1 + num_ai);

    // Player house
    houses_[0].init(player_faction,
                    house_name(player_faction), true);
    houses_[0].set_credits(10000);
    player_house_idx_ = 0;

    // AI houses
    HouseType ai_faction = (player_faction == HouseType::GDI) ?
                            HouseType::Nod : HouseType::GDI;
    for (int i = 0; i < num_ai; i++) {
        houses_[1 + i].init(ai_faction,
                            house_name(ai_faction), false);
        houses_[1 + i].set_credits(10000);
    }
}

void GameSession::update(float dt) {
    if (game_over_) return;

    game_time_ += dt;

    // Update all houses
    for (auto& house : houses_) {
        if (!house.is_defeated()) {
            house.update(dt, entities_);
        }
    }

    // Update entities
    if (tilemap_) {
        entities_.update(dt, *tilemap_);
    }

    // Update combat (find targets, auto-attack)
    update_combat();

    // Update fog of war
    update_fog_of_war();

    // Check win/lose
    check_victory_conditions();
}

House& GameSession::get_house(HouseType type) {
    for (auto& h : houses_) {
        if (h.type() == type) return h;
    }
    return houses_[0]; // Fallback
}

// ---- Commands ----

void GameSession::command_move(const std::vector<EntityID>& units,
                                int target_x, int target_y) {
    if (!tilemap_) return;

    // Convert pixel to cell
    Point target_cell = tilemap_->pixel_to_cell(target_x, target_y);

    for (EntityID id : units) {
        if (Unit* u = entities_.get_unit(id)) {
            Point start_cell(u->cell_x(), u->cell_y());
            SpeedType speed = u->type_data ?
                              u->type_data->speed_type : SpeedType::Track;

            auto path = PathFinder::find_path(
                *tilemap_, start_cell.x, start_cell.y,
                target_cell.x, target_cell.y, speed
            );

            if (!path.empty()) {
                u->path = path;
                u->path_index = 0;
                u->is_moving = true;
                u->mission = MissionType::Move;
            }
        }
        if (Infantry* inf = entities_.get_infantry(id)) {
            Point start_cell(inf->cell_x(), inf->cell_y());

            auto path = PathFinder::find_path(
                *tilemap_, start_cell.x, start_cell.y,
                target_cell.x, target_cell.y, SpeedType::Foot
            );

            if (!path.empty()) {
                inf->path = path;
                inf->path_index = 0;
                inf->is_moving = true;
                inf->mission = MissionType::Move;
            }
        }
    }
}

void GameSession::command_attack(const std::vector<EntityID>& units,
                                  EntityID target) {
    for (EntityID id : units) {
        if (Unit* u = entities_.get_unit(id)) {
            u->attack_target = target;
            u->mission = MissionType::Attack;
        }
        if (Infantry* inf = entities_.get_infantry(id)) {
            inf->attack_target = target;
            inf->mission = MissionType::Attack;
        }
    }
}

void GameSession::command_harvest(EntityID harvester) {
    if (Unit* u = entities_.get_unit(harvester)) {
        if (u->type_data && u->type_data->is_harvester) {
            u->mission = MissionType::Harvest;
        }
    }
}

void GameSession::command_stop(const std::vector<EntityID>& units) {
    for (EntityID id : units) {
        if (Unit* u = entities_.get_unit(id)) {
            u->is_moving = false;
            u->path.clear();
            u->attack_target = INVALID_ENTITY;
            u->mission = MissionType::Guard;
        }
        if (Infantry* inf = entities_.get_infantry(id)) {
            inf->is_moving = false;
            inf->path.clear();
            inf->attack_target = INVALID_ENTITY;
            inf->mission = MissionType::Guard;
        }
    }
}

// ---- Building Placement ----

bool GameSession::can_place_building(BuildingType type,
                                      int cell_x, int cell_y) const {
    if (!tilemap_) return false;

    const auto& data = GameDataRegistry::instance().get_building(type);

    // Check all cells in the building footprint
    for (int dy = 0; dy < data.height_cells; dy++) {
        for (int dx = 0; dx < data.width_cells; dx++) {
            int cx = cell_x + dx;
            int cy = cell_y + dy;

            if (!tilemap_->is_valid(cx, cy)) return false;

            const MapCell& mc = tilemap_->cell(cx, cy);

            // Must be buildable terrain
            if (mc.terrain == TerrainCellType::Water ||
                mc.terrain == TerrainCellType::Rock ||
                mc.terrain == TerrainCellType::Cliff) {
                return false;
            }

            // Must not be occupied
            if (mc.is_occupied()) return false;
        }
    }

    return true;
}

EntityID GameSession::place_building(BuildingType type, HouseType owner,
                                      int cell_x, int cell_y) {
    if (!can_place_building(type, cell_x, cell_y)) return INVALID_ENTITY;

    EntityID id = entities_.create_building(type, owner, cell_x, cell_y);

    Building* bld = entities_.get_building(id);
    if (bld) {
        bld->under_construction = true;
        bld->construction_progress = 0.0f;

        // Mark cells as occupied
        if (tilemap_) {
            for (int dy = 0; dy < bld->height_cells; dy++) {
                for (int dx = 0; dx < bld->width_cells; dx++) {
                    tilemap_->cell(cell_x + dx, cell_y + dy)
                        .occupant_building_id = static_cast<int>(id);
                }
            }
        }
    }

    // Register with house
    for (auto& house : houses_) {
        if (house.type() == owner) {
            house.add_building(id);
            break;
        }
    }

    return id;
}

// ---- Internal Updates ----

void GameSession::update_combat() {
    // Auto-attack: guarding units attack nearby enemies
    for (auto& unit : entities_.mutable_units()) {
        if (unit.mission != MissionType::Guard &&
            unit.mission != MissionType::Attack) continue;
        if (!unit.type_data) continue;
        if (unit.fire_cooldown > 0) continue;

        const WeaponData& weapon = unit.type_data->primary_weapon;
        if (weapon.type == WeaponType::None) continue;

        // If attacking a specific target
        if (unit.mission == MissionType::Attack &&
            unit.attack_target != INVALID_ENTITY) {
            // Check range and fire
            GameEntity* target = nullptr;
            float tx = 0, ty = 0;
            ArmorType target_armor = ArmorType::None;

            if (auto* tu = entities_.get_unit(unit.attack_target)) {
                target = tu; tx = tu->x; ty = tu->y;
                target_armor = tu->type_data ? tu->type_data->armor : ArmorType::None;
            } else if (auto* ti = entities_.get_infantry(unit.attack_target)) {
                target = ti; tx = ti->x; ty = ti->y;
                target_armor = ArmorType::None;
            } else if (auto* tb = entities_.get_building(unit.attack_target)) {
                target = tb; tx = tb->x; ty = tb->y;
                target_armor = tb->type_data ? tb->type_data->armor : ArmorType::Concrete;
            }

            if (target && target->is_alive()) {
                if (CombatSystem::in_range(unit.x, unit.y, tx, ty,
                                            weapon.range)) {
                    // Fire!
                    int damage = CombatSystem::calculate_damage(
                        weapon.type, target_armor, weapon.damage);
                    CombatSystem::apply_damage(*target, damage);
                    unit.fire_cooldown = weapon.rate_of_fire / 15.0f;

                    // Create projectile visual
                    entities_.create_projectile(weapon.type,
                                                unit.x, unit.y, tx, ty,
                                                damage, unit.id);
                } else {
                    // Move closer to target
                    // TODO: pathfind to target
                }
            } else {
                // Target dead, return to guard
                unit.attack_target = INVALID_ENTITY;
                unit.mission = MissionType::Guard;
            }
        }

        // Guard mode: scan for nearby enemies
        if (unit.mission == MissionType::Guard) {
            float scan_range = weapon.range * CELL_PIXEL_WIDTH;

            for (const auto& enemy : entities_.all_units()) {
                if (enemy.owner == unit.owner) continue;
                if (!enemy.is_alive()) continue;

                float dx = enemy.x - unit.x;
                float dy = enemy.y - unit.y;
                if (dx * dx + dy * dy <= scan_range * scan_range) {
                    unit.attack_target = enemy.id;
                    unit.mission = MissionType::Attack;
                    break;
                }
            }
        }
    }
}

void GameSession::update_fog_of_war() {
    if (!tilemap_) return;

    tilemap_->clear_visibility();

    // Reveal around player's units and buildings
    House& player = player_house();

    for (EntityID uid : player.owned_units()) {
        if (Unit* u = entities_.get_unit(uid)) {
            int sight = u->type_data ? u->type_data->sight_range : 4;
            tilemap_->reveal_area(u->cell_x(), u->cell_y(), sight);
        }
        if (Infantry* i = entities_.get_infantry(uid)) {
            int sight = i->type_data ? i->type_data->sight_range : 3;
            tilemap_->reveal_area(i->cell_x(), i->cell_y(), sight);
        }
    }

    for (EntityID bid : player.owned_buildings()) {
        if (Building* b = entities_.get_building(bid)) {
            int sight = b->type_data ? b->type_data->sight_range : 3;
            tilemap_->reveal_area(b->cell_x(), b->cell_y(), sight);
        }
    }
}

void GameSession::check_victory_conditions() {
    int alive_count = 0;
    HouseType last_alive = HouseType::None;

    for (auto& house : houses_) {
        house.check_defeat(entities_);
        if (!house.is_defeated()) {
            alive_count++;
            last_alive = house.type();
        }
    }

    if (alive_count <= 1) {
        game_over_ = true;
        winner_ = last_alive;
    }
}

} // namespace CnC
