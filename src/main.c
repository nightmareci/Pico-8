/** @file main.c
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "emulator.h"

static SDL_Window* window;
static SDL_Renderer* renderer;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("Pico-8", "1.0", "com.pico-8.ngagesdk");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Celeste", NGAGE_W, NGAGE_H, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_S16;
    spec.freq = 8000;

    if (!Mix_OpenAudio(0, &spec))
    {
        SDL_Log("Mix_Init: %s", SDL_GetError());
    }

    if (!init_cart_loader(renderer))
    {
        SDL_Log("Couldn't initialize cartridge loader.");
        return SDL_APP_FAILURE;
    }
    render_selection(renderer, true);

    return SDL_APP_SUCCESS;
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.repeat) // No key repeat.
            {
                break;
            }

            if (event->key.key == SDLK_5 || event->key.key == SDLK_SELECT)
            {
                run_selection(renderer);
                return SDL_APP_CONTINUE;
            }

            if (event->key.key == SDLK_LEFT)
            {
                select_prev(renderer);
                render_selection(renderer, false);
                return SDL_APP_CONTINUE;
            }

            if (event->key.key == SDLK_RIGHT)
            {
                select_next(renderer);
                render_selection(renderer, false);
                return SDL_APP_CONTINUE;
            }

            if (event->key.key == SDLK_SOFTLEFT)
            {
                return SDL_APP_SUCCESS;
            }

            else if (event->key.key == SDLK_HASH);
            {
                render_selection(renderer, true);
            }

            break;
        }
    }

    return SDL_APP_CONTINUE;
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void* appstate)
{
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    destroy_cart_loader();
    Mix_CloseAudio();
    Mix_Quit();
    // SDL will clean up the window/renderer for us.
}
