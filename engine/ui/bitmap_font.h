/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Bitmap Font Renderer
 *
 * Renders text using a programmatically-generated pixel font.
 * Each character is defined as a small bitmap pattern (5x7 pixels).
 * This avoids dependency on SDL_ttf or external font files.
 *
 * The font covers ASCII 32-126 (printable characters).
 */

#pragma once

#include "../core/platform.h"
#include <string>
#include <unordered_map>

namespace CnC {

class BitmapFont {
public:
    BitmapFont() = default;

    // Initialize the font - generates texture atlas
    bool init(Renderer& renderer);
    void shutdown();

    // Render text at position with color and scale
    void draw_text(Renderer& renderer, const std::string& text,
                   int x, int y, const Color& color, int scale = 1) const;

    // Measure text dimensions
    int text_width(const std::string& text, int scale = 1) const;
    int text_height(int scale = 1) const;

    // Draw centered text
    void draw_centered(Renderer& renderer, const std::string& text,
                       int center_x, int y, const Color& color,
                       int scale = 1) const;

private:
    static constexpr int CHAR_WIDTH = 5;
    static constexpr int CHAR_HEIGHT = 7;
    static constexpr int CHAR_SPACING = 1;
    static constexpr int FIRST_CHAR = 32;  // Space
    static constexpr int LAST_CHAR = 126;  // Tilde
    static constexpr int NUM_CHARS = LAST_CHAR - FIRST_CHAR + 1;

    // Each character is a 5x7 bitmap stored as 7 bytes (5 bits each)
    static const uint8_t FONT_DATA[NUM_CHARS][CHAR_HEIGHT];

    TextureID font_texture_ = INVALID_TEXTURE;
    int atlas_width_ = 0;
    int atlas_height_ = 0;
    bool initialized_ = false;
};

} // namespace CnC
