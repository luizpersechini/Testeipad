/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Game Commands & Special Actions
 *
 * Handles:
 *   - MCV deployment (MCV -> Construction Yard and back)
 *   - Unit group assignments (Ctrl+1-9)
 *   - Special unit abilities (engineer capture, commando C4)
 *   - Command feedback (move markers, attack indicators)
 */

#pragma once

#include "game_types.h"
#include "unit.h"
#include "house.h"
#include "../core/platform.h"
#include <array>
#include <vector>

namespace CnC {

// Forward declarations
class GameSession;
class TileMap;

// ============================================================
// Unit Groups (Ctrl+1 through Ctrl+9)
// ============================================================

class UnitGroupManager {
public:
    static constexpr int MAX_GROUPS = 10; // 0-9

    UnitGroupManager() = default;

    // Assign selected units to group
    void assign_group(int group_num, const std::vector<EntityID>& units);

    // Recall a group (select those units)
    std::vector<EntityID> recall_group(int group_num) const;

    // Add to existing group
    void add_to_group(int group_num, const std::vector<EntityID>& units);

    // Remove dead units from all groups
    void cleanup(const EntityManager& entities);

    // Check if group has units
    bool has_group(int group_num) const;
    int group_size(int group_num) const;

private:
    std::array<std::vector<EntityID>, MAX_GROUPS> groups_;
};

// ============================================================
// Command Feedback (visual markers on map)
// ============================================================

struct CommandMarker {
    enum class Type {
        Move,       // Green circle
        Attack,     // Red crosshair
        Harvest,    // Yellow circle
        Deploy,     // Blue square
        Repair      // Wrench icon
    };

    Type type;
    float x, y;
    float lifetime;
    float age = 0.0f;
    Color color;

    bool expired() const { return age >= lifetime; }
    float alpha() const {
        // Fade out in last 30% of lifetime
        float fade_start = lifetime * 0.7f;
        if (age < fade_start) return 1.0f;
        return 1.0f - (age - fade_start) / (lifetime - fade_start);
    }
};

class CommandFeedback {
public:
    CommandFeedback() = default;

    void add_move_marker(float x, float y);
    void add_attack_marker(float x, float y);
    void add_harvest_marker(float x, float y);
    void add_deploy_marker(float x, float y);

    void update(float dt);
    void render(Renderer& renderer, const Rect& viewport);
    void clear() { markers_.clear(); }

private:
    std::vector<CommandMarker> markers_;
    static constexpr float MARKER_LIFETIME = 1.5f;
    static constexpr int MARKER_RADIUS = 8;
};

// ============================================================
// MCV Deployment
// ============================================================

class MCVDeployment {
public:
    // Check if an MCV can deploy at its current location
    static bool can_deploy(const Unit& mcv, const TileMap& tilemap);

    // Deploy MCV into Construction Yard
    // Returns the new building ID, or INVALID_ENTITY on failure
    static EntityID deploy(Unit& mcv, GameSession& session,
                           TileMap& tilemap);

    // Undeploy Construction Yard back into MCV
    static EntityID undeploy(Building& yard, GameSession& session,
                             TileMap& tilemap);

private:
    // Construction Yard footprint (3x3 centered on MCV position)
    static constexpr int YARD_WIDTH = 2;
    static constexpr int YARD_HEIGHT = 2;
};

// ============================================================
// Special Abilities
// ============================================================

class SpecialAbilities {
public:
    // Engineer: capture enemy building
    static bool can_capture(const Infantry& engineer,
                            const Building& target);
    static void capture_building(Infantry& engineer, Building& target,
                                 GameSession& session);

    // Commando: plant C4 on building
    static bool can_c4(const Infantry& commando,
                       const Building& target);
    static void plant_c4(Infantry& commando, Building& target,
                         GameSession& session);

    // Sell building
    static void sell_building(Building& building, House& owner);
    static int sell_value(const Building& building);

    // Repair building
    static bool can_repair(const Building& building);
    static void start_repair(Building& building, House& owner);
    static void update_repair(Building& building, House& owner, float dt);
};

// ============================================================
// Sound Events (for mapping to audio)
// ============================================================

enum class GameSoundEvent {
    // UI
    ButtonClick,
    BuildComplete,
    UnitReady,
    InsufficientFunds,
    CantBuildHere,
    BuildingCancelled,

    // Combat
    Explosion,
    BulletImpact,
    Obelisk,
    IonCannon,
    NukeExplosion,

    // Units
    UnitAcknowledge,
    UnitMoveOrder,
    UnitAttackOrder,
    HarvesterFull,
    MCVDeploy,

    // Alerts
    BaseUnderAttack,
    UnitLost,
    BuildingLost,
    LowPower,
    SilosNeeded,
    MissionAccomplished,
    MissionFailed,

    Count
};

class SoundEventRegistry {
public:
    static SoundEventRegistry& instance();

    void init();

    // Map event to sound file
    void register_sound(GameSoundEvent event, const std::string& file);
    const std::string& get_sound_file(GameSoundEvent event) const;

    // Play sound events
    void play(GameSoundEvent event, AudioSystem* audio);

private:
    SoundEventRegistry() = default;

    struct SoundEntry {
        std::string file;
        SoundID loaded_id = INVALID_SOUND;
    };

    std::array<SoundEntry, static_cast<int>(GameSoundEvent::Count)> sounds_;
    AudioSystem* audio_ = nullptr;
};

} // namespace CnC
