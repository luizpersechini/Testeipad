cmake_minimum_required(VERSION 3.16)
project(CnC_Tiberian_Dawn_Modern VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_WEB "Build for web (Emscripten)" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

# Source files
set(ENGINE_SOURCES
    engine/core/game_engine.cpp
    engine/game/game_data.cpp
    engine/game/unit.cpp
    engine/game/house.cpp
    engine/game/sidebar.cpp
    engine/game/scenario.cpp
    engine/game/ai_player.cpp
    engine/game/commands.cpp
    engine/graphics/sprite.cpp
    engine/graphics/tilemap.cpp
    engine/ui/menu.cpp
    engine/ui/bitmap_font.cpp
)

set(ENGINE_HEADERS
    engine/core/platform.h
    engine/core/game_engine.h
    engine/graphics/sprite.h
    engine/graphics/tilemap.h
    engine/game/game_types.h
    engine/game/unit.h
    engine/game/house.h
    engine/game/sidebar.h
    engine/game/scenario.h
    engine/game/ai_player.h
    engine/game/commands.h
    engine/ui/menu.h
    engine/ui/bitmap_font.h
)

# Platform-specific sources
if(EMSCRIPTEN)
    list(APPEND ENGINE_SOURCES
        engine/core/platform_web.cpp
    )
    add_definitions(-DPLATFORM_WEB=1)
else()
    list(APPEND ENGINE_SOURCES
        engine/core/platform_sdl.cpp
        engine/core/renderer_sdl.cpp
        engine/core/audio_sdl.cpp
    )

    # Find SDL2
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image QUIET)
    find_package(SDL2_mixer QUIET)
endif()

# Main executable
add_executable(cnc_td
    main.cpp
    ${ENGINE_SOURCES}
)

target_include_directories(cnc_td PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/engine
)

if(EMSCRIPTEN)
    # Emscripten build settings
    set_target_properties(cnc_td PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_MIXER=2 \
                    -s SDL2_IMAGE_FORMATS='[\"png\"]' \
                    -s ALLOW_MEMORY_GROWTH=1 \
                    -s TOTAL_MEMORY=67108864 \
                    -s WASM=1 \
                    --preload-file assets \
                    --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
    )
else()
    # Desktop build
    target_link_libraries(cnc_td PRIVATE
        SDL2::SDL2
        SDL2::SDL2main
    )

    # Optional SDL2 extensions
    if(TARGET SDL2_image::SDL2_image)
        target_link_libraries(cnc_td PRIVATE SDL2_image::SDL2_image)
    endif()
    if(TARGET SDL2_mixer::SDL2_mixer)
        target_link_libraries(cnc_td PRIVATE SDL2_mixer::SDL2_mixer)
    endif()
endif()

# Copy assets to build directory
add_custom_command(TARGET cnc_td POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:cnc_td>/assets
    COMMENT "Copying assets to build directory"
)

message(STATUS "C&C Tiberian Dawn Modern Port")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Web build: ${BUILD_WEB}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
