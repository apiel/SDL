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

#ifdef SDL_VIDEO_DRIVER_DUMMY

#include "../SDL_sysvideo.h"
#include "SDL_nullframebuffer_c.h"

#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "spi.h"

#define DISPLAY_SET_CURSOR_X 0x2A
#define DISPLAY_SET_CURSOR_Y 0x2B
#define DISPLAY_WRITE_PIXELS 0x2C

#define BYTESPERPIXEL 2

void sendAddr(uint8_t cmd, uint16_t addr0, uint16_t addr1)
{
    uint8_t addr[4] = { (uint8_t)(addr0 >> 8), (uint8_t)(addr0 & 0xFF), (uint8_t)(addr1 >> 8), (uint8_t)(addr1 & 0xFF) };
    sendCmd(cmd, addr, 4);
}

void drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t data[BYTESPERPIXEL] = { color >> 8, color & 0xFF };

    sendAddr(DISPLAY_SET_CURSOR_X, (uint16_t)x, (uint16_t)x);
    sendAddr(DISPLAY_SET_CURSOR_Y, (uint16_t)y, (uint16_t)y);
    sendCmd(DISPLAY_WRITE_PIXELS, data, 2);
}

void drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    int yPos;
    uint16_t size = w * BYTESPERPIXEL;
    uint8_t pixels[size];
    uint8_t pixel[BYTESPERPIXEL] = { color >> 8, color & 0xFF };

    for (uint16_t i = 0; i < size; i += BYTESPERPIXEL) {
        pixels[i] = pixel[0];
        pixels[i + 1] = pixel[1];
    }

    for (yPos = 0; yPos < h; ++yPos) {
        sendAddr(DISPLAY_SET_CURSOR_X, x, x + w);
        sendAddr(DISPLAY_SET_CURSOR_Y, y + yPos, y + yPos);
        sendCmd(DISPLAY_WRITE_PIXELS, pixels, size);
    }
}

void drawStuff()
{
    int x, y;
    for (y = 0; y < DISPLAY_HEIGHT; ++y) {
        x = DISPLAY_HEIGHT - y - 1;
        drawPixel(x, y, 0xFF00FF);
    }

    // drawFillRect(20, 40, 20, 20, 0xFF00FF);
}

void InitSPIDisplay()
{
    uint8_t madctl = 0;
    uint8_t data[4] = { 0, 0, (uint8_t)(240 >> 8), (uint8_t)(240 & 0xFF) };

    printf("Resetting display at reset GPIO pin %d\n", GPIO_TFT_RESET_PIN);
    SET_GPIO_MODE(GPIO_TFT_RESET_PIN, 1);
    SET_GPIO(GPIO_TFT_RESET_PIN);
    usleep(120 * 1000);
    CLEAR_GPIO(GPIO_TFT_RESET_PIN);
    usleep(120 * 1000);
    SET_GPIO(GPIO_TFT_RESET_PIN);
    usleep(120 * 1000);

    // Do the initialization with a very low SPI bus speed, so that it will succeed even if the bus speed chosen by the user is too high.
    spi->clk = 34;
    __sync_synchronize();

    sendCmdOnly(0x11 /*Sleep Out*/);
    usleep(120 * 1000);
    sendCmdData(0x3A /*COLMOD: Pixel Format Set*/, 0x05 /*16bpp*/);
    usleep(20 * 1000);

#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1 << 6)
#define MADCTL_ROW_ADDRESS_ORDER_SWAP    (1 << 7)
#define MADCTL_ROTATE_180_DEGREES        (MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_ADDRESS_ORDER_SWAP)

    madctl |= MADCTL_ROW_ADDRESS_ORDER_SWAP;
    madctl ^= MADCTL_ROTATE_180_DEGREES;

    sendCmdData(0x36 /*MADCTL: Memory Access Control*/, madctl);
    usleep(10 * 1000);

    sendCmdData(0xBA /*DGMEN: Enable Gamma*/, 0x04);

    sendCmdOnly(0x21 /*Display Inversion On*/);
    // sendCmd(0x20 /*Display Inversion Off*/);

    sendCmdOnly(0x13 /*NORON: Partial off (normal)*/);
    usleep(10 * 1000);

    // The ST7789 controller is actually a unit with 320x240 graphics memory area, but only 240x240 portion
    // of it is displayed. Therefore if we wanted to swap row address mode above, writes to Y=0...239 range will actually land in
    // memory in row addresses Y = 319-(0...239) = 319...80 range. To view this range, we must scroll the view by +80 units in Y
    // direction so that contents of Y=80...319 is displayed instead of Y=0...239.
    if ((madctl & MADCTL_ROW_ADDRESS_ORDER_SWAP)) {
        sendCmd(0x37 /*VSCSAD: Vertical Scroll Start Address of RAM*/, data, 4);
    }

    drawFillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0); // clear screen

    sendCmdOnly(/*Display ON*/ 0x29);
    usleep(100 * 1000);

    // And speed up to the desired operation speed finally after init is done.
    usleep(10 * 1000); // Delay a bit before restoring CLK, or otherwise this has been observed to cause the display not init if done back to back after the clear operation above.
    spi->clk = SPI_BUS_CLOCK_DIVISOR;

    printf("draw stuff\n");
    drawStuff();
}

#define DUMMY_SURFACE "_SDL_DummySurface"

int SDL_DUMMY_CreateWindowFramebuffer(_THIS, SDL_Window *window, Uint32 *format, void **pixels, int *pitch)
{
    SDL_Surface *surface;
    const Uint32 surface_format = SDL_PIXELFORMAT_RGB888;
    int w, h;

    InitSPI();

    printf("Initializing display\n");
    InitSPIDisplay();

    printf("............................................. Creating dummy window framebuffer\n");

    /* Free the old framebuffer surface */
    SDL_DUMMY_DestroyWindowFramebuffer(_this, window);

    /* Create a new one */
    SDL_GetWindowSizeInPixels(window, &w, &h);
    surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 0, surface_format);
    if (!surface) {
        return -1;
    }

    /* Save the info and return! */
    SDL_SetWindowData(window, DUMMY_SURFACE, surface);
    *format = surface_format;
    *pixels = surface->pixels;
    *pitch = surface->pitch;
    return 0;
}

bool yodrawed = false;
int SDL_DUMMY_UpdateWindowFramebuffer(_THIS, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    static int frame_number;
    SDL_Surface *surface;
    Uint8 *bits;
    int i, bw, pad, x, y, w, h;
    uint8_t pixel[BYTESPERPIXEL];
    int pos;
    uint8_t *pixels;

    surface = (SDL_Surface *)SDL_GetWindowData(window, DUMMY_SURFACE);
    if (!surface) {
        return SDL_SetError("Couldn't find dummy surface for window");
    }

    // /* Send the data to the display */
    // if (SDL_getenv("SDL_VIDEO_DUMMY_SAVE_FRAMES")) {
    //     char file[128];
    //     (void)SDL_snprintf(file, sizeof(file), "SDL_window_yoyoyo_%" SDL_PRIu32 "-%8.8d.bmp",
    //                        SDL_GetWindowID(window), ++frame_number);
    //     SDL_SaveBMP(surface, file);
    // }

    // bits = (Uint8 *)surface->pixels + (surface->h * surface->pitch);
    // // pad = ((bw % 4) ? (4 - (bw % 4)) : 0);
    // // while (bits > (Uint8 *)surface->pixels) {
    // //     bits -= surface->pitch;
    // //     drawPixel(0, 0, 0);
    // // }

    // printf("Updating framebuffer\n");
    // if (!yodrawed) {
    //     drawFillRect(20, 40, 20, 20, 0xFF00FF);
    // } else {
    //     drawFillRect(20, 40, 20, 20, 0x00FF00);
    // }
    // yodrawed = !yodrawed;

    w = surface->w > DISPLAY_WIDTH ? DISPLAY_WIDTH : surface->w;
    h = surface->h > DISPLAY_HEIGHT ? DISPLAY_HEIGHT : surface->h;
    for (x = 0; x < w; x++) {
        for (y = 0; y < h; y++) {
            // pos = y * surface->pitch + x ;
            pos = (y * surface->w + x) * surface->format->BitsPerPixel;
            pixels = surface->pixels + pos;
            // pixel[0] = pixels[0];
            // pixel[1] = pixels[1];

            sendAddr(DISPLAY_SET_CURSOR_X, (uint16_t)x, (uint16_t)x);
            sendAddr(DISPLAY_SET_CURSOR_Y, (uint16_t)y, (uint16_t)y);
            // sendCmd(DISPLAY_WRITE_PIXELS, pixel, 2);
            sendCmd(DISPLAY_WRITE_PIXELS, pixels, 2);
        }
    }

    return 0;
}

void SDL_DUMMY_DestroyWindowFramebuffer(_THIS, SDL_Window *window)
{
    SDL_Surface *surface;

    surface = (SDL_Surface *)SDL_SetWindowData(window, DUMMY_SURFACE, NULL);
    SDL_FreeSurface(surface);
}

#endif /* SDL_VIDEO_DRIVER_DUMMY */

/* vi: set ts=4 sw=4 expandtab: */
