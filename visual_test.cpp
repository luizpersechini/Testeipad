/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Visual Test Program
 *
 * Renders game screens (menu, sidebar, game view) to PPM image files
 * using a software renderer. No SDL2 required - just compile and run.
 *
 * Generates:
 *   test_output/01_main_menu.ppm
 *   test_output/02_faction_select.ppm
 *   test_output/03_skirmish_setup.ppm
 *   test_output/04_game_view.ppm
 *   test_output/05_bitmap_font.ppm
 *
 * View with: open test_output/*.ppm  (macOS)
 *            eog test_output/*.ppm   (Linux/GNOME)
 *            feh test_output/*.ppm   (Linux)
 *            Or use any image viewer / browser
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

// Pull in the engine types we need
#include "engine/core/platform.h"
#include "engine/game/game_types.h"
#include "engine/ui/bitmap_font.h"

namespace CnC {

// ============================================================
// Software Renderer (implements Renderer interface for testing)
// ============================================================

class SoftwareRenderer : public Renderer {
public:
    SoftwareRenderer() = default;
    ~SoftwareRenderer() override = default;

    bool init(Platform& /*platform*/, const WindowConfig& config) override {
        width_ = config.logical_width;
        height_ = config.logical_height;
        pixels_.resize(width_ * height_ * 3, 0);
        return true;
    }

    // Standalone init without platform
    bool init_standalone(int width, int height) {
        width_ = width;
        height_ = height;
        pixels_.resize(width_ * height_ * 3, 0);
        return true;
    }

    void shutdown() override { pixels_.clear(); }

    void begin_frame() override {}
    void end_frame() override {}

    void clear(const Color& color) override {
        for (int i = 0; i < width_ * height_; i++) {
            pixels_[i * 3 + 0] = color.r;
            pixels_[i * 3 + 1] = color.g;
            pixels_[i * 3 + 2] = color.b;
        }
    }

    // Texture management - store pixel data in memory
    TextureInfo load_texture(const std::string& /*path*/) override {
        return {INVALID_TEXTURE, 0, 0};
    }

    TextureInfo load_texture_from_memory(const uint8_t* data,
                                          int width, int height,
                                          int channels) override {
        TextureID id = next_tex_id_++;
        StoredTexture tex;
        tex.width = width;
        tex.height = height;
        tex.channels = channels;
        tex.data.assign(data, data + width * height * channels);
        textures_[id] = tex;
        return {id, width, height};
    }

    void destroy_texture(TextureID /*id*/) override {}

    void draw_texture(TextureID /*id*/, const Rect& /*dest*/) override {}
    void draw_texture(TextureID /*id*/, const Rect& /*src*/,
                      const Rect& /*dest*/) override {}

    void draw_texture_colored(TextureID id, const Rect& src,
                               const Rect& dest,
                               const Color& tint) override {
        auto it = textures_.find(id);
        if (it == textures_.end()) return;

        const auto& tex = it->second;

        for (int dy = 0; dy < dest.h; dy++) {
            for (int dx = 0; dx < dest.w; dx++) {
                // Map dest pixel to source pixel
                int sx = src.x + (dx * src.w) / dest.w;
                int sy = src.y + (dy * src.h) / dest.h;

                if (sx < 0 || sx >= tex.width || sy < 0 || sy >= tex.height)
                    continue;

                int src_idx = (sy * tex.width + sx) * tex.channels;
                uint8_t sa = (tex.channels >= 4) ? tex.data[src_idx + 3] : 255;

                if (sa == 0) continue;

                // Apply tint
                uint8_t sr = (tex.data[src_idx + 0] * tint.r) / 255;
                uint8_t sg = (tex.data[src_idx + 1] * tint.g) / 255;
                uint8_t sb = (tex.data[src_idx + 2] * tint.b) / 255;

                int px = dest.x + dx;
                int py = dest.y + dy;
                set_pixel_blended(px, py, sr, sg, sb, sa);
            }
        }
    }

    void draw_rect(const Rect& rect, const Color& color) override {
        // Top
        for (int x = rect.x; x < rect.x + rect.w; x++) {
            set_pixel(x, rect.y, color);
            set_pixel(x, rect.y + rect.h - 1, color);
        }
        // Sides
        for (int y = rect.y; y < rect.y + rect.h; y++) {
            set_pixel(rect.x, y, color);
            set_pixel(rect.x + rect.w - 1, y, color);
        }
    }

    void draw_rect_filled(const Rect& rect, const Color& color) override {
        for (int y = rect.y; y < rect.y + rect.h; y++) {
            for (int x = rect.x; x < rect.x + rect.w; x++) {
                if (color.a < 255) {
                    set_pixel_blended(x, y, color.r, color.g, color.b, color.a);
                } else {
                    set_pixel(x, y, color);
                }
            }
        }
    }

    void draw_line(int x1, int y1, int x2, int y2,
                   const Color& color) override {
        // Bresenham line
        int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            set_pixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x1 += sx; }
            if (e2 <= dx) { err += dx; y1 += sy; }
        }
    }

    // Simple built-in text (5x7 chars drawn as rectangles)
    void draw_text(const std::string& text, int x, int y,
                   const Color& color, int size = 12) override {
        // Use bitmap font if available, else fallback
        if (bitmap_font_) {
            int scale = std::max(1, size / 7);
            bitmap_font_->draw_text(*this, text, x, y, color, scale);
            return;
        }

        // Fallback: simple block characters
        int char_w = std::max(1, size * 5 / 10);
        int char_h = std::max(1, size * 7 / 10);
        int cx = x;

        for (char c : text) {
            if (c == ' ') { cx += char_w + 1; continue; }
            // Draw a small filled rect for each character
            draw_rect_filled(Rect(cx, y, char_w, char_h), color);
            cx += char_w + 1;
        }
    }

    void set_viewport(const Rect& /*viewport*/) override {}
    void set_camera(int x, int y) override {
        camera_x_ = x; camera_y_ = y;
    }

    Point screen_to_world(int screen_x, int screen_y) override {
        return Point(screen_x + camera_x_, screen_y + camera_y_);
    }
    Point world_to_screen(int world_x, int world_y) override {
        return Point(world_x - camera_x_, world_y - camera_y_);
    }

    void set_clip_rect(const Rect& /*rect*/) override {}
    void clear_clip_rect() override {}
    void set_logical_size(int /*width*/, int /*height*/) override {}

    // Save to PPM file
    bool save_ppm(const std::string& filename) const {
        FILE* f = fopen(filename.c_str(), "wb");
        if (!f) return false;
        fprintf(f, "P6\n%d %d\n255\n", width_, height_);
        fwrite(pixels_.data(), 1, pixels_.size(), f);
        fclose(f);
        return true;
    }

    // Set bitmap font for better text rendering
    void set_bitmap_font(BitmapFont* font) { bitmap_font_ = font; }

    // Public pixel access for tests
    void put_pixel(int x, int y, const Color& c) {
        set_pixel(x, y, c);
    }

    int width() const { return width_; }
    int height() const { return height_; }

private:
    void set_pixel(int x, int y, const Color& c) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        int idx = (y * width_ + x) * 3;
        pixels_[idx + 0] = c.r;
        pixels_[idx + 1] = c.g;
        pixels_[idx + 2] = c.b;
    }

    void set_pixel_blended(int x, int y, uint8_t r, uint8_t g, uint8_t b,
                           uint8_t a) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        int idx = (y * width_ + x) * 3;
        float alpha = a / 255.0f;
        pixels_[idx + 0] = static_cast<uint8_t>(r * alpha + pixels_[idx + 0] * (1 - alpha));
        pixels_[idx + 1] = static_cast<uint8_t>(g * alpha + pixels_[idx + 1] * (1 - alpha));
        pixels_[idx + 2] = static_cast<uint8_t>(b * alpha + pixels_[idx + 2] * (1 - alpha));
    }

    struct StoredTexture {
        int width = 0, height = 0, channels = 0;
        std::vector<uint8_t> data;
    };

    int width_ = 0, height_ = 0;
    int camera_x_ = 0, camera_y_ = 0;
    std::vector<uint8_t> pixels_;
    std::unordered_map<TextureID, StoredTexture> textures_;
    TextureID next_tex_id_ = 1;
    BitmapFont* bitmap_font_ = nullptr;
};

} // namespace CnC

// ============================================================
// Drawing helper functions for test screens
// ============================================================

using namespace CnC;

static constexpr int SCREEN_W = 640;
static constexpr int SCREEN_H = 400;

// Draw a mock terrain tile
void draw_tile(SoftwareRenderer& r, int x, int y, int size,
               TerrainCellType terrain, bool has_tiberium = false) {
    Color c;
    switch (terrain) {
        case TerrainCellType::Clear: c = Color(100, 80, 50); break;
        case TerrainCellType::Road:  c = Color(80, 80, 80); break;
        case TerrainCellType::Water: c = Color(20, 40, 120); break;
        case TerrainCellType::Rock:  c = Color(90, 85, 75); break;
        case TerrainCellType::Sand:  c = Color(160, 140, 80); break;
        case TerrainCellType::Rough: c = Color(70, 60, 40); break;
        case TerrainCellType::Cliff: c = Color(60, 55, 50); break;
        default: c = Color(100, 80, 50); break;
    }
    r.draw_rect_filled(Rect(x, y, size, size), c);

    // Subtle grid lines
    r.draw_line(x, y, x + size - 1, y, Color(c.r + 10, c.g + 10, c.b + 10));
    r.draw_line(x, y, x, y + size - 1, Color(c.r + 10, c.g + 10, c.b + 10));

    if (has_tiberium) {
        // Draw tiberium crystals
        int cx = x + size / 2;
        int cy = y + size / 2;
        Color tib(0, 200, 0);
        r.draw_rect_filled(Rect(cx - 2, cy - 4, 3, 8), tib);
        r.draw_rect_filled(Rect(cx - 5, cy - 2, 3, 6), Color(0, 180, 0));
        r.draw_rect_filled(Rect(cx + 3, cy - 3, 3, 7), Color(0, 220, 0));
    }
}

// Draw a mock building
void draw_building(SoftwareRenderer& r, int x, int y, int w, int h,
                   const Color& faction_color, const std::string& label) {
    // Building body
    Color body(faction_color.r / 2, faction_color.g / 2, faction_color.b / 2);
    r.draw_rect_filled(Rect(x, y, w, h), body);

    // Highlight edge
    r.draw_line(x, y, x + w, y, faction_color);
    r.draw_line(x, y, x, y + h, faction_color);

    // Shadow edge
    Color shadow(body.r / 2, body.g / 2, body.b / 2);
    r.draw_line(x + w, y, x + w, y + h, shadow);
    r.draw_line(x, y + h, x + w, y + h, shadow);

    // Label
    r.draw_text(label, x + 2, y + h / 2 - 3, Color::White(), 7);
}

// Draw a mock unit
void draw_unit(SoftwareRenderer& r, int x, int y, int size,
               const Color& faction_color, bool selected = false) {
    // Unit body (tank-like)
    r.draw_rect_filled(Rect(x - size/2, y - size/2, size, size), faction_color);

    // Turret
    int ts = size / 3;
    Color turret(faction_color.r + 30, faction_color.g + 30, faction_color.b + 30);
    r.draw_rect_filled(Rect(x - ts/2, y - ts/2, ts, ts), turret);

    // Gun barrel
    r.draw_line(x, y, x + size/2 + 2, y, Color::White());

    // Selection circle
    if (selected) {
        int sr = size / 2 + 3;
        r.draw_rect(Rect(x - sr, y - sr, sr * 2, sr * 2), Color(0, 255, 0));
        // Health bar
        r.draw_rect_filled(Rect(x - sr, y - sr - 4, sr * 2, 3), Color(0, 180, 0));
    }
}

// Draw infantry
void draw_infantry(SoftwareRenderer& r, int x, int y,
                   const Color& faction_color, bool selected = false) {
    // Body
    r.draw_rect_filled(Rect(x - 2, y - 4, 4, 6), faction_color);
    // Head
    r.draw_rect_filled(Rect(x - 1, y - 6, 3, 3), Color(200, 170, 120));

    if (selected) {
        r.draw_rect(Rect(x - 5, y - 8, 10, 14), Color(0, 255, 0));
    }
}

// ============================================================
// Test Screen 1: Main Menu
// ============================================================

void render_main_menu(SoftwareRenderer& r) {
    r.clear(Color(10, 10, 15));

    int cx = SCREEN_W / 2;

    // Decorative scan lines
    for (int y = 0; y < SCREEN_H; y += 20) {
        r.draw_line(0, y, SCREEN_W, y, Color(30, 30, 40));
    }

    // Title with glow effect
    r.draw_text("COMMAND & CONQUER", cx - 110, 35, Color(255, 220, 0), 20);
    r.draw_text("TIBERIAN DAWN", cx - 80, 65, Color::GDI(), 16);

    // Subtitle
    r.draw_text("MODERN PORT", cx - 42, 92, Color::White(), 10);

    // Divider line
    r.draw_rect_filled(Rect(cx - 120, 108, 240, 2), Color(80, 80, 80));

    // Menu items
    struct MI { const char* label; bool selected; };
    MI items[] = {
        {"CAMPAIGN", true},
        {"SKIRMISH", false},
        {"OPTIONS", false},
        {"QUIT", false}
    };

    int base_y = SCREEN_H / 2 + 20;
    for (int i = 0; i < 4; i++) {
        int iy = base_y + i * 32;
        Rect bounds(cx - 80, iy, 160, 28);

        Color bg = items[i].selected ? Color(40, 40, 60) : Color(20, 20, 25);
        r.draw_rect_filled(bounds, bg);

        if (items[i].selected) {
            r.draw_rect(bounds, Color::GDI());
            r.draw_text(">", bounds.x - 12, iy + 8, Color::GDI(), 10);
        } else {
            r.draw_rect(bounds, Color(50, 50, 50));
        }

        Color tc = items[i].selected ? Color::White() : Color(160, 160, 160);
        int tx = cx - static_cast<int>(strlen(items[i].label)) * 3;
        r.draw_text(items[i].label, tx, iy + 8, tc, 10);
    }

    // Description
    r.draw_text("Play the single-player campaign",
                cx - 110, SCREEN_H - 50, Color(120, 120, 120), 8);

    // Footer
    r.draw_text("Based on C&C: Tiberian Dawn by Westwood Studios",
                cx - 160, SCREEN_H - 20, Color(60, 60, 60), 7);
    r.draw_text("Original source released under GPL v3 by Electronic Arts",
                cx - 175, SCREEN_H - 10, Color(60, 60, 60), 7);
}

// ============================================================
// Test Screen 2: Faction Selection
// ============================================================

void render_faction_select(SoftwareRenderer& r) {
    r.clear(Color(10, 10, 15));
    int cx = SCREEN_W / 2;

    // Scan lines
    for (int y = 0; y < SCREEN_H; y += 20) {
        r.draw_line(0, y, SCREEN_W, y, Color(30, 30, 40));
    }

    r.draw_text("COMMAND & CONQUER", cx - 110, 35, Color(255, 220, 0), 20);
    r.draw_text("TIBERIAN DAWN", cx - 80, 65, Color::GDI(), 16);
    r.draw_text("SELECT YOUR FACTION", cx - 70, 92, Color::White(), 10);
    r.draw_rect_filled(Rect(cx - 120, 108, 240, 2), Color(80, 80, 80));

    // GDI box (selected)
    Rect gdi_box(cx - 145, SCREEN_H / 2 + 10, 130, 80);
    r.draw_rect_filled(gdi_box, Color(40, 40, 50));
    r.draw_rect(gdi_box, Color::GDI());

    // GDI emblem
    Rect gdi_emblem(gdi_box.x + 10, gdi_box.y + 8, gdi_box.w - 20, 24);
    r.draw_rect_filled(gdi_emblem, Color::GDI());
    r.draw_text("GDI", gdi_emblem.x + 40, gdi_emblem.y + 6, Color::Black(), 12);

    // GDI eagle icon (simplified)
    int ex = gdi_box.x + gdi_box.w / 2;
    int ey = gdi_box.y + 55;
    r.draw_line(ex - 15, ey, ex, ey - 10, Color::GDI());
    r.draw_line(ex + 15, ey, ex, ey - 10, Color::GDI());
    r.draw_line(ex - 10, ey + 5, ex, ey - 5, Color::GDI());
    r.draw_line(ex + 10, ey + 5, ex, ey - 5, Color::GDI());

    // Selection arrow
    r.draw_text("^", gdi_box.x + gdi_box.w / 2 - 3,
                gdi_box.y + gdi_box.h + 5, Color::GDI(), 10);

    // GDI description
    r.draw_text("Global Defense Initiative",
                gdi_box.x - 10, gdi_box.y + gdi_box.h + 20,
                Color(140, 140, 140), 7);
    r.draw_text("Heavy armor, Ion Cannon",
                gdi_box.x, gdi_box.y + gdi_box.h + 30,
                Color(140, 140, 140), 7);

    // Nod box
    Rect nod_box(cx + 15, SCREEN_H / 2 + 10, 130, 80);
    r.draw_rect_filled(nod_box, Color(20, 20, 25));
    r.draw_rect(nod_box, Color(60, 60, 60));

    // Nod emblem
    Rect nod_emblem(nod_box.x + 10, nod_box.y + 8, nod_box.w - 20, 24);
    r.draw_rect_filled(nod_emblem, Color::Nod());
    r.draw_text("NOD", nod_emblem.x + 40, nod_emblem.y + 6, Color::Black(), 12);

    // Nod scorpion icon (simplified)
    int nx = nod_box.x + nod_box.w / 2;
    int ny = nod_box.y + 55;
    // Tail curve
    r.draw_line(nx - 8, ny + 5, nx - 5, ny - 5, Color::Nod());
    r.draw_line(nx - 5, ny - 5, nx, ny - 10, Color::Nod());
    r.draw_line(nx, ny - 10, nx + 3, ny - 6, Color::Nod());
    // Body
    r.draw_rect_filled(Rect(nx - 5, ny, 10, 5), Color::Nod());
    // Claws
    r.draw_line(nx + 5, ny, nx + 12, ny - 5, Color::Nod());
    r.draw_line(nx - 5, ny, nx - 12, ny - 5, Color::Nod());

    // Instructions
    r.draw_text("Press ENTER to select, ESC to go back",
                cx - 120, SCREEN_H - 35, Color(80, 80, 80), 8);

    r.draw_text("Based on C&C: Tiberian Dawn by Westwood Studios",
                cx - 160, SCREEN_H - 20, Color(60, 60, 60), 7);
    r.draw_text("Original source released under GPL v3 by Electronic Arts",
                cx - 175, SCREEN_H - 10, Color(60, 60, 60), 7);
}

// ============================================================
// Test Screen 3: Skirmish Setup
// ============================================================

void render_skirmish_setup(SoftwareRenderer& r) {
    r.clear(Color(10, 10, 15));
    int cx = SCREEN_W / 2;

    for (int y = 0; y < SCREEN_H; y += 20) {
        r.draw_line(0, y, SCREEN_W, y, Color(30, 30, 40));
    }

    r.draw_text("COMMAND & CONQUER", cx - 110, 35, Color(255, 220, 0), 20);
    r.draw_text("TIBERIAN DAWN", cx - 80, 65, Color::GDI(), 16);
    r.draw_text("SKIRMISH SETUP", cx - 52, 92, Color::White(), 10);
    r.draw_rect_filled(Rect(cx - 120, 108, 240, 2), Color(80, 80, 80));

    // Faction label
    r.draw_text("PLAYING AS: GDI", cx - 55, SCREEN_H / 2 - 55,
                Color::GDI(), 9);

    // Settings items
    struct Setting { const char* label; bool selected; };
    Setting settings[] = {
        {"MAP: Medium (48x48)", false},
        {"AI: Normal", false},
        {"CREDITS: $10000", false},
        {"FOG OF WAR: ON", false}
    };

    int base_y = SCREEN_H / 2 - 20;
    for (int i = 0; i < 4; i++) {
        int iy = base_y + i * 30;
        Rect bounds(cx - 100, iy, 200, 24);

        Color bg = settings[i].selected ? Color(30, 30, 45) : Color(20, 20, 25);
        r.draw_rect_filled(bounds, bg);

        if (settings[i].selected) {
            r.draw_rect(bounds, Color::GDI());
            r.draw_text("<", bounds.x + 4, iy + 5, Color::GDI(), 10);
            r.draw_text(">", bounds.x + bounds.w - 12, iy + 5, Color::GDI(), 10);
        }

        Color lbl_color = settings[i].selected ? Color::White() : Color(160, 160, 160);
        int tx = cx - static_cast<int>(strlen(settings[i].label)) * 3;
        r.draw_text(settings[i].label, tx, iy + 5, lbl_color, 9);
    }

    // Start button (selected)
    int start_y = base_y + 4 * 30 + 10;
    Rect start_btn(cx - 80, start_y, 160, 30);
    r.draw_rect_filled(start_btn, Color(0, 80, 0));
    r.draw_rect(start_btn, Color(0, 200, 0));
    r.draw_text("START BATTLE", cx - 45, start_y + 9, Color::White(), 10);

    r.draw_text("Based on C&C: Tiberian Dawn by Westwood Studios",
                cx - 160, SCREEN_H - 20, Color(60, 60, 60), 7);
    r.draw_text("Original source released under GPL v3 by Electronic Arts",
                cx - 175, SCREEN_H - 10, Color(60, 60, 60), 7);
}

// ============================================================
// Test Screen 4: In-Game View with Sidebar
// ============================================================

void render_game_view(SoftwareRenderer& r) {
    r.clear(Color(0, 0, 0));

    int sidebar_x = SCREEN_W - 160;

    // --- Terrain ---
    int tile_size = 24;
    // Use a seed for pseudo-random terrain
    auto terrain_at = [](int cx, int cy) -> TerrainCellType {
        int h = (cx * 7 + cy * 13 + cx * cy) % 17;
        if (h < 8) return TerrainCellType::Clear;
        if (h < 11) return TerrainCellType::Sand;
        if (h < 13) return TerrainCellType::Rough;
        if (h == 13) return TerrainCellType::Rock;
        if (h == 14) return TerrainCellType::Water;
        return TerrainCellType::Clear;
    };

    auto has_tib = [](int cx, int cy) -> bool {
        int h = (cx * 3 + cy * 7) % 23;
        return (h < 3) && cx > 6 && cx < 14 && cy > 3 && cy < 10;
    };

    for (int ty = 0; ty < SCREEN_H / tile_size + 1; ty++) {
        for (int tx = 0; tx < sidebar_x / tile_size + 1; tx++) {
            int px = tx * tile_size;
            int py = ty * tile_size;
            if (px >= sidebar_x) continue;
            draw_tile(r, px, py, tile_size, terrain_at(tx, ty), has_tib(tx, ty));
        }
    }

    // --- Buildings ---
    // GDI base (player)
    draw_building(r, 48, 72, 48, 48, Color::GDI(), "CY");
    draw_building(r, 100, 72, 48, 48, Color::GDI(), "PP");
    draw_building(r, 48, 124, 48, 48, Color::GDI(), "REF");
    draw_building(r, 100, 124, 48, 48, Color::GDI(), "BAR");
    draw_building(r, 152, 72, 48, 48, Color::GDI(), "WF");

    // Nod base (enemy, partially visible)
    draw_building(r, 336, 240, 48, 48, Color::Nod(), "CY");
    draw_building(r, 288, 240, 48, 48, Color::Nod(), "PP");
    draw_building(r, 336, 192, 48, 48, Color::Nod(), "HoN");
    draw_building(r, 288, 192, 48, 48, Color::Nod(), "OBL");

    // --- Units ---
    // GDI units (some selected)
    draw_unit(r, 200, 180, 14, Color(140, 120, 60), true);  // Medium tank
    draw_unit(r, 220, 190, 14, Color(140, 120, 60), true);  // Medium tank
    draw_unit(r, 240, 175, 18, Color(160, 140, 80), true);  // Mammoth tank
    draw_unit(r, 175, 200, 12, Color(100, 100, 60), false);  // Humvee

    // Harvester (special)
    r.draw_rect_filled(Rect(130, 180, 16, 12), Color(140, 120, 60));
    r.draw_rect_filled(Rect(130, 175, 16, 5), Color(0, 200, 0));  // Tib load
    r.draw_text("H", 134, 182, Color::White(), 7);

    // Infantry
    draw_infantry(r, 170, 150, Color(140, 120, 60), false);
    draw_infantry(r, 175, 155, Color(140, 120, 60), false);
    draw_infantry(r, 180, 148, Color(140, 120, 60), false);

    // Nod units
    draw_unit(r, 310, 170, 12, Color(140, 40, 40), false);  // Light tank
    draw_unit(r, 330, 165, 12, Color(140, 40, 40), false);  // Buggy

    // --- Command feedback marker ---
    // Move marker (expanding green ring)
    r.draw_rect(Rect(270 - 8, 200 - 8, 16, 16), Color(0, 200, 0, 180));
    r.draw_rect(Rect(270 - 12, 200 - 12, 24, 24), Color(0, 200, 0, 100));

    // --- Drag selection rectangle ---
    // (not shown in still frame)

    // --- Fog of war ---
    // Darken far areas
    for (int ty = 0; ty < SCREEN_H / tile_size + 1; ty++) {
        for (int tx = 0; tx < sidebar_x / tile_size + 1; tx++) {
            int px = tx * tile_size;
            int py = ty * tile_size;
            if (px >= sidebar_x) continue;

            // Distance from player base center (96, 120)
            float dx_f = (px + tile_size/2 - 120.0f);
            float dy_f = (py + tile_size/2 - 140.0f);
            float dist = std::sqrt(dx_f * dx_f + dy_f * dy_f);

            if (dist > 250) {
                // Full fog
                r.draw_rect_filled(Rect(px, py, tile_size, tile_size),
                                   Color(0, 0, 0, 200));
            } else if (dist > 180) {
                // Partial fog (shroud)
                r.draw_rect_filled(Rect(px, py, tile_size, tile_size),
                                   Color(0, 0, 0, 100));
            }
        }
    }

    // ============ SIDEBAR ============
    // Background
    r.draw_rect_filled(Rect(sidebar_x, 0, 160, SCREEN_H), Color(30, 30, 30));
    r.draw_line(sidebar_x, 0, sidebar_x, SCREEN_H, Color(80, 80, 80));

    // Credits
    r.draw_rect_filled(Rect(sidebar_x, 0, 160, 20), Color(20, 20, 20));
    r.draw_text("$8500", sidebar_x + 8, 5, Color(0, 200, 0), 10);

    // Power bar
    int py = 24;
    r.draw_rect_filled(Rect(sidebar_x + 4, py, 12, 100), Color(20, 20, 20));
    r.draw_rect(Rect(sidebar_x + 4, py, 12, 100), Color(60, 60, 60));
    // Green (output)
    r.draw_rect_filled(Rect(sidebar_x + 6, py + 30, 8, 68), Color(0, 180, 0));
    // Red line (drain)
    r.draw_line(sidebar_x + 4, py + 45, sidebar_x + 16, py + 45, Color(255, 0, 0));

    // Minimap
    int mini_x = sidebar_x + 20;
    int mini_y = 24;
    int mini_w = 136;
    int mini_h = 100;
    r.draw_rect_filled(Rect(mini_x, mini_y, mini_w, mini_h), Color(40, 35, 25));
    r.draw_rect(Rect(mini_x, mini_y, mini_w, mini_h), Color(100, 100, 100));

    // Mini terrain dots
    for (int my = 0; my < mini_h - 4; my += 2) {
        for (int mx = 0; mx < mini_w - 4; mx += 2) {
            int cx = mx * 64 / mini_w;
            int cy = my * 64 / mini_h;
            TerrainCellType t = terrain_at(cx, cy);
            Color mc(80, 65, 40);
            if (t == TerrainCellType::Water) mc = Color(20, 40, 100);
            if (t == TerrainCellType::Rock) mc = Color(70, 65, 55);
            if (has_tib(cx, cy)) mc = Color(0, 150, 0);
            r.put_pixel(mini_x + 2 + mx, mini_y + 2 + my, mc);
        }
    }

    // Mini buildings
    r.draw_rect_filled(Rect(mini_x + 4, mini_y + 10, 6, 6), Color::GDI());
    r.draw_rect_filled(Rect(mini_x + 70, mini_y + 50, 6, 6), Color::Nod());

    // Viewport box on minimap
    r.draw_rect(Rect(mini_x + 2, mini_y + 2, 30, 20), Color::White());

    // Tab buttons
    int tab_y = mini_y + mini_h + 4;
    // Structures tab (selected)
    r.draw_rect_filled(Rect(sidebar_x, tab_y, 80, 22), Color(60, 60, 80));
    r.draw_rect(Rect(sidebar_x, tab_y, 80, 22), Color(80, 80, 80));
    r.draw_text("BUILD", sidebar_x + 18, tab_y + 6, Color::White(), 8);

    // Units tab
    r.draw_rect_filled(Rect(sidebar_x + 80, tab_y, 80, 22), Color(40, 40, 40));
    r.draw_rect(Rect(sidebar_x + 80, tab_y, 80, 22), Color(80, 80, 80));
    r.draw_text("UNITS", sidebar_x + 98, tab_y + 6, Color::White(), 8);

    // Build icons
    int icons_y = tab_y + 26;
    struct BuildIconMock {
        const char* name;
        int cost;
        bool building;
        float progress;
        bool ready;
    };
    BuildIconMock icons[] = {
        {"PP",      300,  false, 0.0f, false},
        {"REF",     2000, true,  0.75f, false},
        {"BAR",     300,  false, 0.0f, false},
        {"WF",      2000, false, 0.0f, false},
        {"GT",      500,  false, 0.0f, false},
        {"AGT",     1000, false, 0.0f, false},
        {"SILO",    150,  false, 0.0f, true},
    };

    int icon_w = 76;
    int icon_h = 40;
    for (int i = 0; i < 7; i++) {
        int col = i % 2;
        int row = i / 2;
        int ix = sidebar_x + 2 + col * (icon_w + 2);
        int iy = icons_y + row * (icon_h + 2);

        Color bg = icons[i].ready ? Color(0, 80, 0) : Color(50, 50, 50);
        r.draw_rect_filled(Rect(ix, iy, icon_w, icon_h), bg);

        Color border = icons[i].ready ? Color(0, 200, 0) : Color(100, 100, 100);
        r.draw_rect(Rect(ix, iy, icon_w, icon_h), border);

        r.draw_text(icons[i].name, ix + 3, iy + 3, Color::White(), 7);
        r.draw_text(std::string("$") + std::to_string(icons[i].cost),
                    ix + 3, iy + icon_h - 12, Color(0, 200, 0), 7);

        if (icons[i].building && !icons[i].ready) {
            int bar_w = icon_w - 4;
            int fill_w = static_cast<int>(bar_w * icons[i].progress);
            r.draw_rect_filled(Rect(ix + 2, iy + icon_h / 2, bar_w, 4),
                               Color(40, 40, 40));
            r.draw_rect_filled(Rect(ix + 2, iy + icon_h / 2, fill_w, 4),
                               Color(0, 200, 200));
        }

        if (icons[i].ready) {
            r.draw_text("READY", ix + 3, iy + icon_h / 2 - 4,
                        Color(0, 255, 0), 8);
        }
    }

    // Superweapon bar at bottom
    int sw_y = SCREEN_H - 28;
    r.draw_rect_filled(Rect(sidebar_x, sw_y, 160, 26), Color(20, 20, 20));
    int bar_w = 152;
    int fill_w = static_cast<int>(bar_w * 0.4f);
    r.draw_rect_filled(Rect(sidebar_x + 4, sw_y + 4, bar_w, 8),
                       Color(40, 40, 40));
    r.draw_rect_filled(Rect(sidebar_x + 4, sw_y + 4, fill_w, 8),
                       Color(0, 100, 200));
    r.draw_text("CHARGING", sidebar_x + 4, sw_y + 14,
                Color(0, 100, 200), 7);

    // --- HUD overlay on game area ---
    // Unit group indicators
    r.draw_text("GROUP 1: 3 units", 4, SCREEN_H - 12,
                Color(0, 200, 0), 7);
}

// ============================================================
// Test Screen 5: Bitmap Font Showcase
// ============================================================

void render_font_test(SoftwareRenderer& r) {
    r.clear(Color(15, 15, 20));

    int y = 10;

    r.draw_text("BITMAP FONT TEST", 10, y, Color::White(), 14);
    y += 20;

    r.draw_rect_filled(Rect(10, y, SCREEN_W - 20, 1), Color(60, 60, 60));
    y += 8;

    // Full character set
    r.draw_text("Full ASCII Range:", 10, y, Color(120, 120, 120), 7);
    y += 12;

    r.draw_text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 10, y, Color::White(), 10);
    y += 14;
    r.draw_text("abcdefghijklmnopqrstuvwxyz", 10, y, Color::White(), 10);
    y += 14;
    r.draw_text("0123456789", 10, y, Color::White(), 10);
    y += 14;
    r.draw_text("!@#$%^&*()-=+[]{}|;:',.<>?/~`\"\\", 10, y, Color::White(), 10);
    y += 20;

    // Scale comparison
    r.draw_text("Scale Comparison:", 10, y, Color(120, 120, 120), 7);
    y += 12;

    r.draw_text("Scale 1x: The quick brown fox", 10, y, Color::White(), 7);
    y += 12;
    r.draw_text("Scale 2x: COMMAND & CONQUER", 10, y, Color::GDI(), 14);
    y += 22;
    r.draw_text("Scale 3x: C&C TD", 10, y, Color(255, 220, 0), 21);
    y += 30;

    // Color samples
    r.draw_text("Color Samples:", 10, y, Color(120, 120, 120), 7);
    y += 12;

    r.draw_text("GDI GOLD", 10, y, Color::GDI(), 10);
    r.draw_text("NOD RED", 100, y, Color::Nod(), 10);
    r.draw_text("TIBERIUM", 180, y, Color::Tiberium(), 10);
    r.draw_text("WARNING!", 270, y, Color::Red(), 10);
    r.draw_text("$10000", 360, y, Color(0, 200, 0), 10);
    y += 16;

    // Game-like text
    r.draw_rect_filled(Rect(10, y, SCREEN_W - 20, 1), Color(60, 60, 60));
    y += 8;
    r.draw_text("In-Game Text Samples:", 10, y, Color(120, 120, 120), 7);
    y += 14;

    // Credits display
    r.draw_rect_filled(Rect(10, y, 100, 16), Color(20, 20, 20));
    r.draw_text("$8500", 14, y + 3, Color(0, 200, 0), 10);
    y += 22;

    // Build notification
    r.draw_rect_filled(Rect(10, y, 200, 16), Color(0, 40, 0));
    r.draw_text("Construction Complete", 14, y + 3, Color(0, 255, 0), 10);
    y += 22;

    // Warning
    r.draw_rect_filled(Rect(10, y, 200, 16), Color(60, 0, 0));
    r.draw_text("Base Under Attack!", 14, y + 3, Color(255, 100, 100), 10);
    y += 22;

    // Unit names
    r.draw_text("Mammoth Tank | Medium Tank | Harvester | MCV",
                10, y, Color(180, 180, 180), 8);
    y += 14;
    r.draw_text("Minigunner | Rocket Soldier | Engineer | Commando",
                10, y, Color(180, 180, 180), 8);
    y += 14;
    r.draw_text("Construction Yard | Power Plant | Barracks | Refinery",
                10, y, Color(180, 180, 180), 8);
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    printf("C&C Tiberian Dawn - Modern Port Visual Test\n");
    printf("=============================================\n\n");

    // Create output directory
    mkdir("test_output", 0755);

    // Create renderer
    SoftwareRenderer renderer;
    renderer.init_standalone(SCREEN_W, SCREEN_H);

    // Initialize bitmap font
    BitmapFont font;
    bool font_ok = font.init(renderer);
    if (font_ok) {
        renderer.set_bitmap_font(&font);
        printf("[OK] Bitmap font initialized (5x7 pixel, %d characters)\n", 95);
    } else {
        printf("[!!] Bitmap font failed to initialize, using fallback\n");
    }

    // Initialize game data registry (needed for some screens)
    GameDataRegistry::instance().init();
    printf("[OK] Game data registry initialized\n");

    // Render test screens
    printf("\nRendering test screens...\n\n");

    // Screen 1: Main Menu
    renderer.clear(Color::Black());
    render_main_menu(renderer);
    renderer.save_ppm("test_output/01_main_menu.ppm");
    printf("  [1/5] Main Menu          -> test_output/01_main_menu.ppm\n");

    // Screen 2: Faction Selection
    renderer.clear(Color::Black());
    render_faction_select(renderer);
    renderer.save_ppm("test_output/02_faction_select.ppm");
    printf("  [2/5] Faction Selection   -> test_output/02_faction_select.ppm\n");

    // Screen 3: Skirmish Setup
    renderer.clear(Color::Black());
    render_skirmish_setup(renderer);
    renderer.save_ppm("test_output/03_skirmish_setup.ppm");
    printf("  [3/5] Skirmish Setup      -> test_output/03_skirmish_setup.ppm\n");

    // Screen 4: In-Game View
    renderer.clear(Color::Black());
    render_game_view(renderer);
    renderer.save_ppm("test_output/04_game_view.ppm");
    printf("  [4/5] In-Game View        -> test_output/04_game_view.ppm\n");

    // Screen 5: Font showcase
    renderer.clear(Color::Black());
    render_font_test(renderer);
    renderer.save_ppm("test_output/05_bitmap_font.ppm");
    printf("  [5/5] Bitmap Font Test    -> test_output/05_bitmap_font.ppm\n");

    printf("\nAll test screens rendered successfully!\n");
    printf("View images with any image viewer or browser.\n");
    printf("\nTo convert to PNG: convert test_output/01_main_menu.ppm test_output/01_main_menu.png\n");

    font.shutdown();
    renderer.shutdown();

    return 0;
}
