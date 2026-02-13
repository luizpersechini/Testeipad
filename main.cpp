/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Main Entry Point
 *
 * Based on the original C&C: Tiberian Dawn by Westwood Studios.
 * Original source code released under GPL v3 by Electronic Arts.
 *
 * This modern port targets PC (Windows/macOS/Linux) and Web browsers.
 */

#include "engine/core/game_engine.h"
#include <cstdio>

int main(int argc, char* argv[]) {
    CnC::GameConfig config;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-f" || arg == "--fullscreen") {
            config.window.fullscreen = true;
        } else if (arg == "--fps") {
            config.show_fps = true;
        } else if (arg == "--width" && i + 1 < argc) {
            config.window.width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            config.window.height = std::stoi(argv[++i]);
        } else if (arg == "--speed" && i + 1 < argc) {
            config.game_speed = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            printf("C&C: Tiberian Dawn - Modern Port\n\n");
            printf("Options:\n");
            printf("  -f, --fullscreen    Start in fullscreen mode\n");
            printf("  --fps               Show FPS counter\n");
            printf("  --width <W>         Window width (default: 1280)\n");
            printf("  --height <H>        Window height (default: 720)\n");
            printf("  --speed <0-7>       Game speed (default: 5)\n");
            printf("  -h, --help          Show this help\n");
            return 0;
        }
    }

    CnC::GameEngine engine;

    if (!engine.init(config)) {
        fprintf(stderr, "Failed to initialize game engine.\n");
        return 1;
    }

    engine.run();
    engine.shutdown();

    return 0;
}
