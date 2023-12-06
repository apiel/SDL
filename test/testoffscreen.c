/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

/* Simple program: picks the offscreen backend and renders each frame to a bmp */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// #include "SDL.h"
// #include "SDL_stdinc.h"
// #include "SDL_opengl.h"

#include "../include/SDL.h"
#include "../include/SDL_opengl.h"
#include "../include/SDL_stdinc.h"

static SDL_Renderer *renderer = NULL;
static SDL_Window *window = NULL;
static int done = SDL_FALSE;
static int width = 640;
static int height = 480;

void draw()
{
    SDL_Rect Rect;

    SDL_SetRenderDrawColor(renderer, 0x10, 0x9A, 0xCE, 0xFF);
    SDL_RenderClear(renderer);

    /* Grow based on the frame just to show a difference per frame of the region */
    Rect.x = 20;
    Rect.y = 20;
    Rect.w = 100;
    Rect.h = 100;
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x10, 0x21, 0xFF);
    SDL_RenderFillRect(renderer, &Rect);

    SDL_RenderPresent(renderer);
}

void save_surface_to_bmp()
{
    SDL_Surface *surface;
    Uint32 r_mask, g_mask, b_mask, a_mask;
    Uint32 pixel_format;
    char file[128];
    int bbp;

    pixel_format = SDL_GetWindowPixelFormat(window);
    SDL_PixelFormatEnumToMasks(pixel_format, &bbp, &r_mask, &g_mask, &b_mask, &a_mask);

    surface = SDL_CreateRGBSurface(0, width, height, bbp, r_mask, g_mask, b_mask, a_mask);
    SDL_RenderReadPixels(renderer, NULL, pixel_format, (void *)surface->pixels, surface->pitch);

    SDL_snprintf(file, sizeof(file), "SDL_window%" SDL_PRIs32 "-%8.8d.bmp", SDL_GetWindowID(window), 0);

    SDL_SaveBMP(surface, file);
    SDL_FreeSurface(surface);
}

void loop()
{
    SDL_Event event;

    /* Check for events */
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            done = SDL_TRUE;
            break;
        }
    }

    draw();
    save_surface_to_bmp();
}

int main(int argc, char *argv[])
{
    Uint32 then, now, frames;

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    /* Force the offscreen renderer, if it cannot be created then fail out */
    if (SDL_VideoInit("offscreen") < 0) {
        SDL_Log("Couldn't initialize the waveshare video driver: %s\n",
                SDL_GetError());
        return SDL_FALSE;
    }

    /* If OPENGL fails to init it will fallback to using a framebuffer for rendering */
    window = SDL_CreateWindow("Waveshare Test",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height, 0);

    if (!window) {
        SDL_Log("Couldn't create window: %s\n", SDL_GetError());
        return SDL_FALSE;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (!renderer) {
        SDL_Log("Couldn't create renderer: %s\n",
                SDL_GetError());
        return SDL_FALSE;
    }

    SDL_RenderClear(renderer);

    loop();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
