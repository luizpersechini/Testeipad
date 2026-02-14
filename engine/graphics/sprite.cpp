/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Sprite Manager Implementation
 */

#include "sprite.h"
#include <fstream>

namespace CnC {

// ============================================================
// SpriteInstance
// ============================================================

SpriteInstance::SpriteInstance(const SpriteDef* def)
    : def_(def) {
}

void SpriteInstance::set_animation(AnimationType type) {
    if (!def_) return;
    if (type == current_anim_) return;

    current_anim_ = type;
    current_frame_index_ = 0;
    anim_timer_ = 0.0f;
    anim_complete_ = false;
}

void SpriteInstance::set_facing(int facing) {
    current_facing_ = facing & 7; // Clamp to 0-7
}

void SpriteInstance::update(float dt) {
    if (!def_ || anim_complete_) return;

    auto it = def_->animations.find(current_anim_);
    if (it == def_->animations.end()) return;

    const AnimationDef& anim = it->second;
    if (anim.frame_count <= 1) return;

    anim_timer_ += dt * 1000.0f; // Convert to ms

    if (anim_timer_ >= anim.frame_duration_ms) {
        anim_timer_ -= anim.frame_duration_ms;
        current_frame_index_++;

        if (current_frame_index_ >= anim.frame_count) {
            if (anim.loop) {
                current_frame_index_ = 0;
            } else {
                current_frame_index_ = anim.frame_count - 1;
                anim_complete_ = true;
            }
        }
    }
}

void SpriteInstance::reset() {
    current_frame_index_ = 0;
    anim_timer_ = 0.0f;
    anim_complete_ = false;
}

int SpriteInstance::get_current_frame() const {
    if (!def_) return 0;

    auto it = def_->animations.find(current_anim_);
    int base_frame = 0;
    if (it != def_->animations.end()) {
        base_frame = it->second.start_frame + current_frame_index_;
    }

    // Add facing offset
    if (def_->has_facings) {
        base_frame += current_facing_ * def_->frames_per_facing;
    }

    return base_frame;
}

Rect SpriteInstance::get_current_source_rect() const {
    if (!def_) return Rect();
    int frame = get_current_frame();
    return def_->sheet.get_frame_rect(frame);
}

TextureID SpriteInstance::get_texture() const {
    if (!def_) return INVALID_TEXTURE;
    return def_->sheet.texture;
}

bool SpriteInstance::is_animation_complete() const {
    return anim_complete_;
}

// ============================================================
// SpriteManager
// ============================================================

bool SpriteManager::init(Renderer& renderer, const std::string& base_path) {
    renderer_ = &renderer;
    base_path_ = base_path;
    return true;
}

void SpriteManager::shutdown() {
    for (auto& [path, tex_id] : texture_cache_) {
        if (renderer_ && tex_id != INVALID_TEXTURE) {
            renderer_->destroy_texture(tex_id);
        }
    }
    texture_cache_.clear();
    sprite_defs_.clear();
}

bool SpriteManager::load_sprite_sheet(const std::string& id,
                                       const std::string& image_path,
                                       int frame_width, int frame_height) {
    TextureID tex = load_or_get_texture(image_path);
    if (tex == INVALID_TEXTURE) return false;

    SpriteDef def;
    def.id = id;
    def.sheet_path = image_path;
    def.sheet.texture = tex;
    def.sheet.frame_width = frame_width;
    def.sheet.frame_height = frame_height;

    // Get texture dimensions to compute columns/rows
    // For now, assume single-frame if we don't know dimensions
    def.sheet.columns = 1;
    def.sheet.rows = 1;

    // Default idle animation
    AnimationDef idle;
    idle.name = "idle";
    idle.type = AnimationType::Idle;
    idle.start_frame = 0;
    idle.frame_count = 1;
    idle.loop = true;
    def.animations[AnimationType::Idle] = idle;

    sprite_defs_[id] = def;
    return true;
}

void SpriteManager::register_sprite(const SpriteDef& def) {
    sprite_defs_[def.id] = def;
}

const SpriteDef* SpriteManager::get_sprite_def(const std::string& id) const {
    auto it = sprite_defs_.find(id);
    if (it != sprite_defs_.end()) return &it->second;
    return nullptr;
}

SpriteInstance SpriteManager::create_instance(const std::string& sprite_id) const {
    const SpriteDef* def = get_sprite_def(sprite_id);
    return SpriteInstance(def);
}

void SpriteManager::draw_sprite(const SpriteInstance& sprite, int x, int y,
                                 const Color& tint) {
    if (!renderer_ || !sprite.definition()) return;

    TextureID tex = sprite.get_texture();
    if (tex == INVALID_TEXTURE) return;

    Rect src = sprite.get_current_source_rect();
    Rect dest(x - src.w / 2, y - src.h / 2, src.w, src.h);

    renderer_->draw_texture_colored(tex, src, dest, tint);
}

void SpriteManager::draw_sprite_frame(const std::string& sprite_id,
                                       int frame, int x, int y,
                                       const Color& tint) {
    const SpriteDef* def = get_sprite_def(sprite_id);
    if (!def || !renderer_) return;

    TextureID tex = def->sheet.texture;
    if (tex == INVALID_TEXTURE) return;

    Rect src = def->sheet.get_frame_rect(frame);
    Rect dest(x - src.w / 2, y - src.h / 2, src.w, src.h);

    renderer_->draw_texture_colored(tex, src, dest, tint);
}

void SpriteManager::begin_batch() {
    // Batch rendering placeholder - in a real implementation,
    // this would set up instanced rendering or a sprite batch buffer
}

void SpriteManager::batch_sprite(const SpriteInstance& sprite, int x, int y,
                                  const Color& tint) {
    // For now, just draw immediately
    draw_sprite(sprite, x, y, tint);
}

void SpriteManager::end_batch() {
    // Flush batch buffer
}

void SpriteManager::generate_faction_variant(const std::string& /*sprite_id*/,
                                              const Color& /*faction_color*/) {
    // TODO: Implement runtime faction color remapping
    // This would modify texture pixels to swap faction-colored regions
}

TextureID SpriteManager::load_or_get_texture(const std::string& path) {
    // Check cache
    auto it = texture_cache_.find(path);
    if (it != texture_cache_.end()) return it->second;

    // Try loading from base_path + path, then just path
    std::string full_path = base_path_ + "/" + path;
    TextureInfo info = renderer_->load_texture(full_path);

    if (info.id == INVALID_TEXTURE) {
        info = renderer_->load_texture(path);
    }

    if (info.id != INVALID_TEXTURE) {
        texture_cache_[path] = info.id;
    }

    return info.id;
}

} // namespace CnC
