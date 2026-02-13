/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * SDL2_mixer Audio Implementation
 *
 * Handles sound effects and music playback using SDL2_mixer.
 * Supports the original C&C soundtrack and sound effects.
 */

#include "platform.h"

#ifndef PLATFORM_WEB

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <unordered_map>

namespace CnC {

class AudioSystemSDL : public AudioSystem {
public:
    ~AudioSystemSDL() override { shutdown(); }

    bool init() override {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            return false;
        }
        Mix_AllocateChannels(32); // 32 sound channels
        return true;
    }

    void shutdown() override {
        for (auto& [id, chunk] : sounds_) {
            Mix_FreeChunk(chunk);
        }
        sounds_.clear();

        for (auto& [id, music] : music_tracks_) {
            Mix_FreeMusic(music);
        }
        music_tracks_.clear();

        Mix_CloseAudio();
    }

    SoundID load_sound(const std::string& path) override {
        Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
        if (!chunk) return INVALID_SOUND;

        SoundID id = next_sound_id_++;
        sounds_[id] = chunk;
        return id;
    }

    void play_sound(SoundID id, float volume, float pan) override {
        auto it = sounds_.find(id);
        if (it == sounds_.end()) return;

        int channel = Mix_PlayChannel(-1, it->second, 0);
        if (channel >= 0) {
            Mix_Volume(channel,
                       static_cast<int>(volume * MIX_MAX_VOLUME));

            // Pan: -1.0 (left) to 1.0 (right)
            uint8_t left = static_cast<uint8_t>(
                (1.0f - pan) * 0.5f * 255);
            uint8_t right = static_cast<uint8_t>(
                (1.0f + pan) * 0.5f * 255);
            Mix_SetPanning(channel, left, right);
        }
    }

    void stop_sound(SoundID /*id*/) override {
        // SDL_mixer doesn't track which channel plays which sound easily
        // For now, this is a no-op. A real implementation would
        // track channel-to-sound mappings.
    }

    MusicID load_music(const std::string& path) override {
        Mix_Music* music = Mix_LoadMUS(path.c_str());
        if (!music) return INVALID_MUSIC;

        MusicID id = next_music_id_++;
        music_tracks_[id] = music;
        return id;
    }

    void play_music(MusicID id, bool loop) override {
        auto it = music_tracks_.find(id);
        if (it == music_tracks_.end()) return;

        Mix_PlayMusic(it->second, loop ? -1 : 1);
    }

    void stop_music() override {
        Mix_HaltMusic();
    }

    void set_music_volume(float volume) override {
        Mix_VolumeMusic(static_cast<int>(volume * MIX_MAX_VOLUME));
    }

    void set_sound_volume(float volume) override {
        // Set volume for all channels
        Mix_Volume(-1, static_cast<int>(volume * MIX_MAX_VOLUME));
    }

    bool is_music_playing() const override {
        return Mix_PlayingMusic() != 0;
    }

private:
    SoundID next_sound_id_ = 1;
    MusicID next_music_id_ = 1;
    std::unordered_map<SoundID, Mix_Chunk*> sounds_;
    std::unordered_map<MusicID, Mix_Music*> music_tracks_;
};

std::unique_ptr<AudioSystem> AudioSystem::create() {
    return std::make_unique<AudioSystemSDL>();
}

} // namespace CnC

#endif // !PLATFORM_WEB
