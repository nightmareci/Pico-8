/** @file image_loader.c
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include "image_loader.h"

#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Texture* load_image(SDL_Renderer* renderer, const char* file_name, int* width, int* height, int* bpp)
{
    FILE* file = fopen(file_name, "rb");
    if (!file)
    {
        SDL_Log("Couldn't open file: %s", file_name);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    Uint8* data = (Uint8*)SDL_calloc(file_size, sizeof(Uint8));
    if (!data)
    {
        SDL_Log("Couldn't allocate memory for image file data");
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, file_size, file) != file_size)  {
        SDL_Log("Couldn't read memory for image file data");
        fclose(file);
        return NULL;
    }
    fclose(file);

    Uint32* image_data = (Uint32*)stbi_load_from_memory(data, file_size, width, height, bpp, 4);
    if (!image_data)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        return NULL;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(*width, *height, SDL_PIXELFORMAT_ABGR8888, image_data, *width * 4);
    if (!surface)
    {
        SDL_Log("Couldn't create surface: %s", SDL_GetError());
        stbi_image_free(image_data);
        return NULL;
    }

    SDL_Texture* image = SDL_CreateTextureFromSurface(renderer, surface);
    if (!image)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        return NULL;
    }
    SDL_DestroySurface(surface);
    stbi_image_free(image_data);

    return image;
}
