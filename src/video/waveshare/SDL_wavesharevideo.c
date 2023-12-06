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

/* Waveshare video driver is similar to dummy driver, however its purpose
 * is enabling applications to use some of the SDL video functionality
 * (notably context creation) while not requiring a display output.
 *
 * An example would be running a graphical program on a headless box
 * for automated testing.
 */

#include "SDL_video.h"

#include "SDL_waveshareevents_c.h"
#include "SDL_waveshareframebuffer_c.h"
#include "SDL_wavesharevideo.h"
#include "SDL_wavesharewindow.h"

#include "lib/Config/DEV_Config.h"
#include "lib/LCD/LCD_1in47.h"

#define WAVESHAREVID_DRIVER_NAME "waveshare"

/* Initialization/Query functions */
static int WAVESHARE_VideoInit(_THIS);
static int WAVESHARE_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);
static void WAVESHARE_VideoQuit(_THIS);

/* WAVESHARE driver bootstrap functions */

static void WAVESHARE_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

static SDL_VideoDevice *WAVESHARE_CreateDevice(void)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return 0;
    }

    /* General video */
    device->VideoInit = WAVESHARE_VideoInit;
    device->VideoQuit = WAVESHARE_VideoQuit;
    device->SetDisplayMode = WAVESHARE_SetDisplayMode;
    device->PumpEvents = WAVESHARE_PumpEvents;
    device->CreateWindowFramebuffer = SDL_WAVESHARE_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SDL_WAVESHARE_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SDL_WAVESHARE_DestroyWindowFramebuffer;
    device->free = WAVESHARE_DeleteDevice;

    /* "Window" */
    device->CreateSDLWindow = WAVESHARE_CreateWindow;
    device->DestroyWindow = WAVESHARE_DestroyWindow;

    return device;
}

VideoBootStrap WAVESHARE_bootstrap = {
    WAVESHAREVID_DRIVER_NAME, "SDL waveshare video driver",
    WAVESHARE_CreateDevice
};

int WAVESHARE_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    /* Use a fake 32-bpp desktop mode */
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = LCD_1IN47_HEIGHT;
    mode.h = LCD_1IN47_WIDTH;
    // mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_zero(mode);
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    /* Module Init */
    if (DEV_ModuleInit() != 0) {
        return -1;
    }

    /* LCD Init */
    printf("1.47inch LCD demo...\r\n");
    LCD_1IN47_Init(HORIZONTAL);
    LCD_1IN47_Clear(BLACK);
    LCD_SetBacklight(1023);

    UDOUBLE Imagesize = LCD_1IN47_HEIGHT * LCD_1IN47_WIDTH * 2;
    if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        return -1;
    }
    /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage(BlackImage, LCD_1IN47_WIDTH, LCD_1IN47_HEIGHT, 90, BLACK, 16);

    /* We're done! */
    return 0;
}

static int WAVESHARE_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return 0;
}

void WAVESHARE_VideoQuit(_THIS)
{
}

#endif /* SDL_VIDEO_DRIVER_WAVESHARE */

/* vi: set ts=4 sw=4 expandtab: */
