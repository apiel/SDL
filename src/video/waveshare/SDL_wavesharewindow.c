/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_WAVESHARE

#include "../SDL_sysvideo.h"
#include "../SDL_egl_c.h"

#include "SDL_wavesharewindow.h"

int WAVESHARE_CreateWindow(_THIS, SDL_Window *window)
{
    WAVESHARE_Window *waveshare_window = SDL_calloc(1, sizeof(WAVESHARE_Window));

    if (!waveshare_window) {
        return SDL_OutOfMemory();
    }

    window->driverdata = waveshare_window;

    if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        window->x = 0;
    }

    if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        window->y = 0;
    }

    waveshare_window->sdl_window = window;

    return 0;
}

void WAVESHARE_DestroyWindow(_THIS, SDL_Window *window)
{
    WAVESHARE_Window *waveshare_window = window->driverdata;

    if (waveshare_window) {
        SDL_free(waveshare_window);
    }

    window->driverdata = NULL;
}

#endif /* SDL_VIDEO_DRIVER_WAVESHARE */

/* vi: set ts=4 sw=4 expandtab: */
