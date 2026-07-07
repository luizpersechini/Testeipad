/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Sprite Management System
 *
 * Handles loading, caching, and rendering of game sprites.
 * Supports sprite sheets, animations, and faction-colored variants.
 * Integrates with the AI asset pipeline for sprite generation.
 */

#pragma once

#include "../core/platform.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace CnC {

// ============================================================
// Sprite Frame
// ============================================================

struct SpriteFrame {
    TextureID texture = INVALID_TEXTURE;
    Rect source_rect;          // Region within the texture/sheet
    Point offset;              // Rendering offset from origin
    int duration_ms = 100;     // Frame duration for animations
};

// ============================================================
// Sprite Sheet
// ============================================================

struct SpriteSheet {
    TextureID texture = INVALID_TEXTURE;
    int texture_width = 0;
    int texture_height = 0;
    int frame_width = 0;
    int frame_height = 0;
    int columns = 0;
    int rows = 0;

    Rect get_frame_rect(int frame_index) const {
        int col = frame_index % columns;
        int row = frame_index / columns;
        return Rect(col * frame_width, row * frame_height,
                    frame_width, frame_height);
    }

    int total_frames() const { return columns * rows; }
};

// ============================================================
// Animation Definition
// ============================================================

enum class AnimationType {
    Idle,
    Moving,
    Attacking,
    Dying,
    Building,       // Construction animation
    Harvesting,
    Turret,         // Turret rotation
    Garrison,       // Building with garrison
    Count
};

struct AnimationDef {
    std::string name;
    AnimationType type = AnimationType::Idle;
    int start_frame = 0;
    int frame_count = 1;
    int frame_duration_ms = 100;
    bool loop = true;
    bool ping_pong = false;   // Play forward then backward
};

// ============================================================
// Sprite Definition
// ============================================================

struct SpriteDef {
    std::string id;            // Unique identifier (e.g., "unit_mtank")
    std::string sheet_path;    // Path to sprite sheet image
    SpriteSheet sheet;

    // Animations indexed by type
    std::unordered_map<AnimationType, AnimationDef> animations;

    // For directional sprites (8 facings)
    bool has_facings = false;
    int frames_per_facing = 1;

    // Faction color remapping
    bool supports_faction_colors = true;

    // Shadow
    bool has_shadow = false;
    int shadow_offset_x = 2;
    int shadow_offset_y = 2;
};

// ============================================================
// Sprite Instance (runtime state for an animated sprite)
// ============================================================

class SpriteInstance {
public:
    SpriteInstance() = default;
    explicit SpriteInstance(const SpriteDef* def);

    void set_animation(AnimationType type);
    void set_facing(int facing);  // 0-7 for 8 directions
    void update(float dt);
    void reset();

    int get_current_frame() const;
    Rect get_current_source_rect() const;
    TextureID get_texture() const;
    bool is_animation_complete() const;

    const SpriteDef* definition() const { return def_; }

private:
    const SpriteDef* def_ = nullptr;
    AnimationType current_anim_ = AnimationType::Idle;
    int current_facing_ = 0;
    float anim_timer_ = 0.0f;
    int current_frame_index_ = 0;
    bool anim_complete_ = false;
};

// ============================================================
// Sprite Manager
// ============================================================

class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    bool init(Renderer& renderer, const std::string& base_path);
    void shutdown();

    // Loading
    bool load_sprite_def(const std::string& id, const std::string& json_path);
    bool load_sprite_sheet(const std::string& id, const std::string& image_path,
                           int frame_width, int frame_height);

    // Register a sprite definition programmatically
    void register_sprite(const SpriteDef& def);

    // Access
    const SpriteDef* get_sprite_def(const std::string& id) const;
    SpriteInstance create_instance(const std::string& sprite_id) const;

    // Rendering helpers
    void draw_sprite(const SpriteInstance& sprite, int x, int y,
                     const Color& tint = Color::White());
    void draw_sprite_frame(const std::string& sprite_id, int frame,
                           int x, int y,
                           const Color& tint = Color::White());

    // Batch rendering (more efficient for many sprites)
    void begin_batch();
    void batch_sprite(const SpriteInstance& sprite, int x, int y,
                      const Color& tint = Color::White());
    void end_batch();

    // Faction color variants
    void generate_faction_variant(const std::string& sprite_id,
                                  const Color& faction_color);

    // Stats
    int loaded_count() const { return static_cast<int>(sprite_defs_.size()); }

private:
    Renderer* renderer_ = nullptr;
    std::string base_path_;
    std::unordered_map<std::string, SpriteDef> sprite_defs_;
    std::unordered_map<std::string, TextureID> texture_cache_;

    TextureID load_or_get_texture(const std::string& path);
};

} // namespace CnC
