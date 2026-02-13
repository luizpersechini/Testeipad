/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * SDL2 + OpenGL Renderer Implementation
 *
 * Handles all rendering using SDL2's renderer with optional OpenGL
 * acceleration. Supports texture management, sprite batching,
 * and scaled rendering for the original 640x400 resolution.
 */

#include "platform.h"

#ifndef PLATFORM_WEB

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <unordered_map>
#include <vector>

namespace CnC {

class RendererSDL : public Renderer {
public:
    ~RendererSDL() override { shutdown(); }

    bool init(Platform& platform, const WindowConfig& config) override {
        // Get SDL window from platform (platform is PlatformSDL)
        auto* sdl_platform = dynamic_cast<PlatformSDL*>(&platform);
        if (!sdl_platform) return false;

        uint32_t flags = SDL_RENDERER_ACCELERATED;
        if (config.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

        sdl_renderer_ = SDL_CreateRenderer(
            sdl_platform->get_window(), -1, flags
        );

        if (!sdl_renderer_) return false;

        SDL_SetRenderDrawBlendMode(sdl_renderer_, SDL_BLENDMODE_BLEND);

        // Initialize SDL_image
        int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
        IMG_Init(img_flags);

        config_ = config;
        camera_x_ = 0;
        camera_y_ = 0;
        next_texture_id_ = 1;

        return true;
    }

    void shutdown() override {
        // Destroy all textures
        for (auto& [id, tex] : textures_) {
            if (tex) SDL_DestroyTexture(tex);
        }
        textures_.clear();

        if (sdl_renderer_) {
            SDL_DestroyRenderer(sdl_renderer_);
            sdl_renderer_ = nullptr;
        }

        IMG_Quit();
    }

    void begin_frame() override {
        // Nothing specific needed for SDL renderer
    }

    void end_frame() override {
        SDL_RenderPresent(sdl_renderer_);
    }

    void clear(const Color& color) override {
        SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g,
                               color.b, color.a);
        SDL_RenderClear(sdl_renderer_);
    }

    TextureInfo load_texture(const std::string& path) override {
        TextureInfo info;
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) return info;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(
            sdl_renderer_, surface
        );

        if (texture) {
            info.id = next_texture_id_++;
            info.width = surface->w;
            info.height = surface->h;
            textures_[info.id] = texture;
            texture_sizes_[info.id] = {surface->w, surface->h};
        }

        SDL_FreeSurface(surface);
        return info;
    }

    TextureInfo load_texture_from_memory(const uint8_t* data,
                                          int width, int height,
                                          int channels) override {
        TextureInfo info;

        uint32_t rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000; gmask = 0x00ff0000;
        bmask = 0x0000ff00; amask = 0x000000ff;
#else
        rmask = 0x000000ff; gmask = 0x0000ff00;
        bmask = 0x00ff0000; amask = 0xff000000;
#endif

        int pitch = width * channels;
        int depth = channels * 8;

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            const_cast<uint8_t*>(data), width, height, depth, pitch,
            rmask, gmask, bmask, (channels == 4) ? amask : 0
        );

        if (!surface) return info;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(
            sdl_renderer_, surface
        );

        if (texture) {
            info.id = next_texture_id_++;
            info.width = width;
            info.height = height;
            textures_[info.id] = texture;
            texture_sizes_[info.id] = {width, height};
        }

        SDL_FreeSurface(surface);
        return info;
    }

    void destroy_texture(TextureID id) override {
        auto it = textures_.find(id);
        if (it != textures_.end()) {
            SDL_DestroyTexture(it->second);
            textures_.erase(it);
            texture_sizes_.erase(id);
        }
    }

    void draw_texture(TextureID id, const Rect& dest) override {
        auto it = textures_.find(id);
        if (it == textures_.end()) return;

        SDL_Rect dst = {dest.x - camera_x_, dest.y - camera_y_,
                        dest.w, dest.h};
        SDL_RenderCopy(sdl_renderer_, it->second, nullptr, &dst);
    }

    void draw_texture(TextureID id, const Rect& src,
                      const Rect& dest) override {
        auto it = textures_.find(id);
        if (it == textures_.end()) return;

        SDL_Rect s = {src.x, src.y, src.w, src.h};
        SDL_Rect d = {dest.x - camera_x_, dest.y - camera_y_,
                      dest.w, dest.h};
        SDL_RenderCopy(sdl_renderer_, it->second, &s, &d);
    }

    void draw_texture_colored(TextureID id, const Rect& src,
                               const Rect& dest,
                               const Color& tint) override {
        auto it = textures_.find(id);
        if (it == textures_.end()) return;

        SDL_SetTextureColorMod(it->second, tint.r, tint.g, tint.b);
        SDL_SetTextureAlphaMod(it->second, tint.a);

        SDL_Rect s = {src.x, src.y, src.w, src.h};
        SDL_Rect d = {dest.x - camera_x_, dest.y - camera_y_,
                      dest.w, dest.h};
        SDL_RenderCopy(sdl_renderer_, it->second, &s, &d);

        // Reset color mod
        SDL_SetTextureColorMod(it->second, 255, 255, 255);
        SDL_SetTextureAlphaMod(it->second, 255);
    }

    void draw_rect(const Rect& rect, const Color& color) override {
        SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g,
                               color.b, color.a);
        SDL_Rect r = {rect.x - camera_x_, rect.y - camera_y_,
                      rect.w, rect.h};
        SDL_RenderDrawRect(sdl_renderer_, &r);
    }

    void draw_rect_filled(const Rect& rect, const Color& color) override {
        SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g,
                               color.b, color.a);
        SDL_Rect r = {rect.x - camera_x_, rect.y - camera_y_,
                      rect.w, rect.h};
        SDL_RenderFillRect(sdl_renderer_, &r);
    }

    void draw_line(int x1, int y1, int x2, int y2,
                  const Color& color) override {
        SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g,
                               color.b, color.a);
        SDL_RenderDrawLine(sdl_renderer_,
                           x1 - camera_x_, y1 - camera_y_,
                           x2 - camera_x_, y2 - camera_y_);
    }

    void draw_text(const std::string& text, int x, int y,
                  const Color& color, int size) override {
        // Basic text rendering using rectangles as placeholder
        // A real implementation would use SDL_ttf or a bitmap font
        int cx = x - camera_x_;
        int cy = y - camera_y_;
        int char_w = size / 2;
        int char_h = size;

        SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g,
                               color.b, color.a);

        for (size_t i = 0; i < text.size(); i++) {
            if (text[i] != ' ') {
                SDL_Rect r = {cx + static_cast<int>(i) * (char_w + 1),
                             cy, char_w, char_h};
                SDL_RenderFillRect(sdl_renderer_, &r);
            }
        }
    }

    void set_viewport(const Rect& viewport) override {
        SDL_Rect vp = {viewport.x, viewport.y, viewport.w, viewport.h};
        SDL_RenderSetViewport(sdl_renderer_, &vp);
    }

    void set_camera(int x, int y) override {
        camera_x_ = x;
        camera_y_ = y;
    }

    Point screen_to_world(int screen_x, int screen_y) override {
        return Point(screen_x + camera_x_, screen_y + camera_y_);
    }

    Point world_to_screen(int world_x, int world_y) override {
        return Point(world_x - camera_x_, world_y - camera_y_);
    }

    void set_logical_size(int width, int height) override {
        SDL_RenderSetLogicalSize(sdl_renderer_, width, height);
    }

private:
    // Forward declare PlatformSDL for access
    class PlatformSDL;

    SDL_Renderer* sdl_renderer_ = nullptr;
    WindowConfig config_;
    int camera_x_ = 0;
    int camera_y_ = 0;

    TextureID next_texture_id_ = 1;
    std::unordered_map<TextureID, SDL_Texture*> textures_;
    std::unordered_map<TextureID, Point> texture_sizes_;
};

// Forward-declare PlatformSDL class for renderer access
class PlatformSDL;

std::unique_ptr<Renderer> Renderer::create() {
    return std::make_unique<RendererSDL>();
}

} // namespace CnC

#endif // !PLATFORM_WEB
