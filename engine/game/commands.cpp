/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Game Commands Implementation
 */

#include "commands.h"
#include "../graphics/tilemap.h"
#include <algorithm>
#include <cmath>

namespace CnC {

// ============================================================
// UnitGroupManager
// ============================================================

void UnitGroupManager::assign_group(int group_num,
                                     const std::vector<EntityID>& units) {
    if (group_num < 0 || group_num >= MAX_GROUPS) return;
    groups_[group_num] = units;
}

std::vector<EntityID> UnitGroupManager::recall_group(int group_num) const {
    if (group_num < 0 || group_num >= MAX_GROUPS) return {};
    return groups_[group_num];
}

void UnitGroupManager::add_to_group(int group_num,
                                     const std::vector<EntityID>& units) {
    if (group_num < 0 || group_num >= MAX_GROUPS) return;
    for (EntityID id : units) {
        if (std::find(groups_[group_num].begin(),
                      groups_[group_num].end(), id) == groups_[group_num].end()) {
            groups_[group_num].push_back(id);
        }
    }
}

void UnitGroupManager::cleanup(const EntityManager& entities) {
    for (auto& group : groups_) {
        group.erase(
            std::remove_if(group.begin(), group.end(),
                [&](EntityID id) {
                    const auto* u = const_cast<EntityManager&>(entities).get_unit(id);
                    if (u) return !u->is_alive();
                    const auto* i = const_cast<EntityManager&>(entities).get_infantry(id);
                    if (i) return !i->is_alive();
                    return true; // Not found
                }),
            group.end()
        );
    }
}

bool UnitGroupManager::has_group(int group_num) const {
    if (group_num < 0 || group_num >= MAX_GROUPS) return false;
    return !groups_[group_num].empty();
}

int UnitGroupManager::group_size(int group_num) const {
    if (group_num < 0 || group_num >= MAX_GROUPS) return 0;
    return static_cast<int>(groups_[group_num].size());
}

// ============================================================
// CommandFeedback
// ============================================================

void CommandFeedback::add_move_marker(float x, float y) {
    CommandMarker m;
    m.type = CommandMarker::Type::Move;
    m.x = x; m.y = y;
    m.lifetime = MARKER_LIFETIME;
    m.color = Color(0, 200, 0);
    markers_.push_back(m);
}

void CommandFeedback::add_attack_marker(float x, float y) {
    CommandMarker m;
    m.type = CommandMarker::Type::Attack;
    m.x = x; m.y = y;
    m.lifetime = MARKER_LIFETIME;
    m.color = Color(255, 0, 0);
    markers_.push_back(m);
}

void CommandFeedback::add_harvest_marker(float x, float y) {
    CommandMarker m;
    m.type = CommandMarker::Type::Harvest;
    m.x = x; m.y = y;
    m.lifetime = MARKER_LIFETIME;
    m.color = Color(200, 200, 0);
    markers_.push_back(m);
}

void CommandFeedback::add_deploy_marker(float x, float y) {
    CommandMarker m;
    m.type = CommandMarker::Type::Deploy;
    m.x = x; m.y = y;
    m.lifetime = MARKER_LIFETIME * 1.5f;
    m.color = Color(0, 100, 255);
    markers_.push_back(m);
}

void CommandFeedback::update(float dt) {
    for (auto& m : markers_) {
        m.age += dt;
    }
    markers_.erase(
        std::remove_if(markers_.begin(), markers_.end(),
            [](const CommandMarker& m) { return m.expired(); }),
        markers_.end()
    );
}

void CommandFeedback::render(Renderer& renderer, const Rect& viewport) {
    for (const auto& m : markers_) {
        int sx = static_cast<int>(m.x) - viewport.x;
        int sy = static_cast<int>(m.y) - viewport.y;

        float a = m.alpha();
        uint8_t alpha = static_cast<uint8_t>(a * 200);

        Color c(m.color.r, m.color.g, m.color.b, alpha);

        // Expanding ring effect
        float expand = m.age / m.lifetime;
        int r = MARKER_RADIUS + static_cast<int>(expand * 8);

        switch (m.type) {
            case CommandMarker::Type::Move:
                // Green circle
                renderer.draw_rect(
                    Rect(sx - r, sy - r, r * 2, r * 2), c);
                break;

            case CommandMarker::Type::Attack:
                // Red crosshair
                renderer.draw_line(sx - r, sy, sx + r, sy, c);
                renderer.draw_line(sx, sy - r, sx, sy + r, c);
                renderer.draw_rect(
                    Rect(sx - r/2, sy - r/2, r, r), c);
                break;

            case CommandMarker::Type::Harvest:
                // Yellow diamond
                renderer.draw_line(sx, sy - r, sx + r, sy, c);
                renderer.draw_line(sx + r, sy, sx, sy + r, c);
                renderer.draw_line(sx, sy + r, sx - r, sy, c);
                renderer.draw_line(sx - r, sy, sx, sy - r, c);
                break;

            case CommandMarker::Type::Deploy:
                // Blue expanding square
                renderer.draw_rect(
                    Rect(sx - r, sy - r, r * 2, r * 2), c);
                renderer.draw_rect(
                    Rect(sx - r/2, sy - r/2, r, r), c);
                break;

            case CommandMarker::Type::Repair:
                renderer.draw_rect_filled(
                    Rect(sx - 2, sy - r, 4, r * 2), c);
                renderer.draw_rect_filled(
                    Rect(sx - r, sy - 2, r * 2, 4), c);
                break;
        }
    }
}

// ============================================================
// MCV Deployment
// ============================================================

bool MCVDeployment::can_deploy(const Unit& mcv, const TileMap& tilemap) {
    if (mcv.type != UnitType::MCV) return false;
    if (!mcv.is_alive()) return false;

    int cx = mcv.cell_x();
    int cy = mcv.cell_y();

    // Check that all cells in the yard footprint are available
    for (int dy = 0; dy < YARD_HEIGHT; dy++) {
        for (int dx = 0; dx < YARD_WIDTH; dx++) {
            int px = cx + dx;
            int py = cy + dy;

            if (!tilemap.is_valid(px, py)) return false;

            const MapCell& cell = tilemap.cell(px, py);

            // Must be buildable terrain
            if (cell.terrain == TerrainCellType::Water ||
                cell.terrain == TerrainCellType::Rock ||
                cell.terrain == TerrainCellType::Cliff ||
                cell.terrain == TerrainCellType::Tiberium) {
                return false;
            }

            // Must not be occupied (except by the MCV itself)
            if (cell.occupant_building_id >= 0) return false;
            if (cell.occupant_unit_id >= 0 &&
                cell.occupant_unit_id != static_cast<int>(mcv.id)) {
                return false;
            }
        }
    }

    return true;
}

EntityID MCVDeployment::deploy(Unit& mcv, GameSession& session,
                                TileMap& tilemap) {
    if (!can_deploy(mcv, tilemap)) return INVALID_ENTITY;

    int cx = mcv.cell_x();
    int cy = mcv.cell_y();
    HouseType owner = mcv.owner;

    // Remove the MCV unit
    House& house = session.get_house(owner);
    house.remove_unit(mcv.id);
    session.entities().destroy_unit(mcv.id);

    // Create Construction Yard at MCV position
    EntityID yard_id = session.place_building(BuildingType::HQ, owner, cx, cy);

    // Mark it as immediately constructed (deployed, not built)
    Building* yard = session.entities().get_building(yard_id);
    if (yard) {
        yard->under_construction = false;
        yard->construction_progress = 1.0f;
    }

    return yard_id;
}

EntityID MCVDeployment::undeploy(Building& yard, GameSession& session,
                                  TileMap& tilemap) {
    if (yard.type != BuildingType::HQ) return INVALID_ENTITY;
    if (!yard.is_alive()) return INVALID_ENTITY;

    int cx = yard.cell_x();
    int cy = yard.cell_y();
    HouseType owner = yard.owner;

    // Remove building
    House& house = session.get_house(owner);
    house.remove_building(yard.id);

    // Clear occupied cells
    const auto& data = GameDataRegistry::instance().get_building(BuildingType::HQ);
    for (int dy = 0; dy < data.height_cells; dy++) {
        for (int dx = 0; dx < data.width_cells; dx++) {
            if (tilemap.is_valid(cx + dx, cy + dy)) {
                tilemap.cell(cx + dx, cy + dy).occupant_building_id = -1;
            }
        }
    }

    session.entities().destroy_building(yard.id);

    // Create MCV at the same position
    float px = static_cast<float>(cx * CELL_PIXEL_WIDTH + CELL_PIXEL_WIDTH / 2);
    float py = static_cast<float>(cy * CELL_PIXEL_HEIGHT + CELL_PIXEL_HEIGHT / 2);

    EntityID mcv_id = session.entities().create_unit(UnitType::MCV, owner, px, py);
    if (mcv_id != INVALID_ENTITY) {
        house.add_unit(mcv_id);
    }

    return mcv_id;
}

// ============================================================
// Special Abilities
// ============================================================

bool SpecialAbilities::can_capture(const Infantry& engineer,
                                    const Building& target) {
    if (!engineer.type_data || !engineer.type_data->is_engineer) return false;
    if (!engineer.is_alive() || !target.is_alive()) return false;
    if (engineer.owner == target.owner) return false;

    // Must be adjacent
    float dx = engineer.x - target.x;
    float dy = engineer.y - target.y;
    float dist = std::sqrt(dx*dx + dy*dy);

    return dist < CELL_PIXEL_WIDTH * 2;
}

void SpecialAbilities::capture_building(Infantry& engineer, Building& target,
                                         GameSession& session) {
    if (!can_capture(engineer, target)) return;

    HouseType old_owner = target.owner;
    HouseType new_owner = engineer.owner;

    // Transfer building ownership
    House& old_house = session.get_house(old_owner);
    House& new_house = session.get_house(new_owner);

    old_house.remove_building(target.id);
    target.owner = new_owner;
    new_house.add_building(target.id);

    // Engineer is consumed
    session.entities().destroy_infantry(engineer.id);
    new_house.remove_unit(engineer.id);
}

bool SpecialAbilities::can_c4(const Infantry& commando,
                               const Building& target) {
    if (!commando.type_data || !commando.type_data->can_c4) return false;
    if (!commando.is_alive() || !target.is_alive()) return false;
    if (commando.owner == target.owner) return false;

    float dx = commando.x - target.x;
    float dy = commando.y - target.y;
    float dist = std::sqrt(dx*dx + dy*dy);

    return dist < CELL_PIXEL_WIDTH * 2;
}

void SpecialAbilities::plant_c4(Infantry& commando, Building& target,
                                 GameSession& session) {
    if (!can_c4(commando, target)) return;

    // C4 instantly destroys the building
    target.hit_points = 0;

    House& owner = session.get_house(target.owner);
    owner.remove_building(target.id);
    session.entities().destroy_building(target.id);

    // Record the kill
    House& attacker = session.get_house(commando.owner);
    attacker.record_building_kill();
}

void SpecialAbilities::sell_building(Building& building, House& owner) {
    if (building.selling) return;
    building.selling = true;
    building.sell_progress = 0.0f;
}

int SpecialAbilities::sell_value(const Building& building) {
    if (!building.type_data) return 0;
    // Sell for 50% of cost, scaled by health
    int base_value = building.type_data->cost / 2;
    return static_cast<int>(base_value * building.health_ratio());
}

bool SpecialAbilities::can_repair(const Building& building) {
    return building.is_alive() &&
           !building.under_construction &&
           !building.selling &&
           building.health_ratio() < 1.0f;
}

void SpecialAbilities::start_repair(Building& building, House& owner) {
    if (!can_repair(building)) return;
    building.repairing = true;
}

void SpecialAbilities::update_repair(Building& building, House& owner,
                                      float dt) {
    if (!building.repairing || !building.is_alive()) return;

    // Repair costs credits over time
    int repair_cost_per_sec = 5;
    if (!owner.spend_credits(static_cast<int>(repair_cost_per_sec * dt))) {
        building.repairing = false; // Can't afford
        return;
    }

    // Repair rate: ~2% HP per second
    float repair_rate = building.max_hit_points * 0.02f;
    building.hit_points += static_cast<int>(repair_rate * dt);

    if (building.hit_points >= building.max_hit_points) {
        building.hit_points = building.max_hit_points;
        building.repairing = false;
    }
}

// ============================================================
// SoundEventRegistry
// ============================================================

SoundEventRegistry& SoundEventRegistry::instance() {
    static SoundEventRegistry inst;
    return inst;
}

void SoundEventRegistry::init() {
    // Map sound events to files
    // These use the naming convention from the original C&C sound archive
    register_sound(GameSoundEvent::ButtonClick, "sounds/button1.wav");
    register_sound(GameSoundEvent::BuildComplete, "sounds/speech/building.wav");
    register_sound(GameSoundEvent::UnitReady, "sounds/speech/unitredy.wav");
    register_sound(GameSoundEvent::InsufficientFunds, "sounds/speech/nocash1.wav");
    register_sound(GameSoundEvent::CantBuildHere, "sounds/speech/cantbild.wav");
    register_sound(GameSoundEvent::BuildingCancelled, "sounds/speech/cancel1.wav");

    register_sound(GameSoundEvent::Explosion, "sounds/explsion.wav");
    register_sound(GameSoundEvent::BulletImpact, "sounds/gun5.wav");
    register_sound(GameSoundEvent::Obelisk, "sounds/obelisk1.wav");
    register_sound(GameSoundEvent::IonCannon, "sounds/ion1.wav");
    register_sound(GameSoundEvent::NukeExplosion, "sounds/nukexplo.wav");

    register_sound(GameSoundEvent::UnitAcknowledge, "sounds/ackno.wav");
    register_sound(GameSoundEvent::UnitMoveOrder, "sounds/roger.wav");
    register_sound(GameSoundEvent::UnitAttackOrder, "sounds/affirm1.wav");
    register_sound(GameSoundEvent::HarvesterFull, "sounds/speech/silofull.wav");
    register_sound(GameSoundEvent::MCVDeploy, "sounds/constru2.wav");

    register_sound(GameSoundEvent::BaseUnderAttack, "sounds/speech/baseatk1.wav");
    register_sound(GameSoundEvent::UnitLost, "sounds/speech/unitlost.wav");
    register_sound(GameSoundEvent::BuildingLost, "sounds/speech/strclost.wav");
    register_sound(GameSoundEvent::LowPower, "sounds/speech/lopower1.wav");
    register_sound(GameSoundEvent::SilosNeeded, "sounds/speech/silneed1.wav");
    register_sound(GameSoundEvent::MissionAccomplished, "sounds/speech/accom1.wav");
    register_sound(GameSoundEvent::MissionFailed, "sounds/speech/fail1.wav");
}

void SoundEventRegistry::register_sound(GameSoundEvent event,
                                          const std::string& file) {
    int idx = static_cast<int>(event);
    if (idx >= 0 && idx < static_cast<int>(GameSoundEvent::Count)) {
        sounds_[idx].file = file;
    }
}

const std::string& SoundEventRegistry::get_sound_file(GameSoundEvent event) const {
    static const std::string empty;
    int idx = static_cast<int>(event);
    if (idx >= 0 && idx < static_cast<int>(GameSoundEvent::Count)) {
        return sounds_[idx].file;
    }
    return empty;
}

void SoundEventRegistry::play(GameSoundEvent event, AudioSystem* audio) {
    if (!audio) return;

    int idx = static_cast<int>(event);
    if (idx < 0 || idx >= static_cast<int>(GameSoundEvent::Count)) return;

    auto& entry = sounds_[idx];
    if (entry.file.empty()) return;

    // Lazy-load the sound
    if (entry.loaded_id == INVALID_SOUND) {
        entry.loaded_id = audio->load_sound(entry.file);
    }

    if (entry.loaded_id != INVALID_SOUND) {
        audio->play_sound(entry.loaded_id);
    }
}

} // namespace CnC
