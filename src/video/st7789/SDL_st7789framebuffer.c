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

#ifdef SDL_VIDEO_DRIVER_ST7789

#include "../SDL_sysvideo.h"
#include "SDL_st7789framebuffer_c.h"

#include <stdio.h>
#include <unistd.h>

#include "SDL_st7789spi.h"
#include "config.h"

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

void InitSPIDisplay()
{
    uint8_t madctl = 0;
    uint8_t data[4] = { 0, 0, (uint8_t)(240 >> 8), (uint8_t)(240 & 0xFF) };

    printf("Resetting display at reset GPIO pin %d\n", GPIO_TFT_RESET_PIN);
    setGpioMode(GPIO_TFT_RESET_PIN, 1);
    setGpio(GPIO_TFT_RESET_PIN);
    usleep(120 * 1000);
    clearGpio(GPIO_TFT_RESET_PIN);
    usleep(120 * 1000);
    setGpio(GPIO_TFT_RESET_PIN);
    usleep(120 * 1000);

    printf("Initializing SPI bus\n");

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
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08

    madctl |= MADCTL_ROW_ADDRESS_ORDER_SWAP;
    madctl |= ST7789_MADCTL_BGR; // Because we swap order, we need to use BGR...
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

    setGpioMode(GPIO_TFT_BACKLIGHT, 1);
    setGpio(GPIO_TFT_BACKLIGHT);

    sendCmdOnly(/*Display ON*/ 0x29);
    usleep(100 * 1000);

    // And speed up to the desired operation speed finally after init is done.
    usleep(10 * 1000); // Delay a bit before restoring CLK, or otherwise this has been observed to cause the display not init if done back to back after the clear operation above.
    spi->clk = SPI_BUS_CLOCK_DIVISOR;
}

#define ST7789_SURFACE "_SDL_DummySurface"

int SDL_ST7789_CreateWindowFramebuffer(_THIS, SDL_Window *window, Uint32 *format, void **pixels, int *pitch)
{
    SDL_Surface *surface;
    const Uint32 surface_format = SDL_PIXELFORMAT_RGB888;
    int w, h;

    InitSPI();

    printf("Initializing display\n");
    InitSPIDisplay();

    /* Free the old framebuffer surface */
    SDL_ST7789_DestroyWindowFramebuffer(_this, window);

    /* Create a new one */
    SDL_GetWindowSizeInPixels(window, &w, &h);
    surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 0, surface_format);
    if (!surface) {
        return -1;
    }

    // SDL_PIXELFORMAT_RGB888 => SDL_PIXELFORMAT_XRGB8888
    printf("............... Creating ST7789 window framebuffer w %d h %d BytesPerPixel %d format %d %s\n", w, h, surface->format->BytesPerPixel, surface->format->format, surface->format->palette ? "palette" : "no palette");

    /* Save the info and return! */
    SDL_SetWindowData(window, ST7789_SURFACE, surface);
    *format = surface_format;
    *pixels = surface->pixels;
    *pitch = surface->pitch;
    return 0;
}

int SDL_ST7789_UpdateWindowFramebuffer(_THIS, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SDL_Surface *surface;
    int x, y, w, h;
    int pos;
    uint8_t *pixels;
    uint16_t rgb;
    uint16_t size;
    uint8_t pixelsBuffer[2048];

    surface = (SDL_Surface *)SDL_GetWindowData(window, ST7789_SURFACE);
    if (!surface) {
        return SDL_SetError("Couldn't find st7789 surface for window");
    }

    w = surface->w > DISPLAY_WIDTH ? DISPLAY_WIDTH : surface->w;
    h = surface->h > DISPLAY_HEIGHT ? DISPLAY_HEIGHT : surface->h;

    size = w * BYTESPERPIXEL;
    if (size > sizeof(pixelsBuffer)) {
        return SDL_SetError("pixels buffer too small");
    }

    // draw row by row
    for (x = 0; x < w; x++) {
        for (y = 0; y < h; y++) {
            pos = (y * surface->w + x) * surface->format->BytesPerPixel;
            pixels = surface->pixels + pos;

            // rgb = (((pixels[0] >> 3) << 11) | ((pixels[1] >> 2) << 5) | (pixels[2] >> 3));
            rgb = ((pixels[0] & 0xF8) << 8) | ((pixels[1] & 0xFC) << 3) | (pixels[2] >> 3);
            pixelsBuffer[(y * BYTESPERPIXEL)] = (uint8_t)(rgb >> 8);
            pixelsBuffer[(y * BYTESPERPIXEL) + 1] = (uint8_t)(rgb & 0xFF);
        }

        // Let's rotate 90 degrees
        sendAddr(DISPLAY_SET_CURSOR_Y, (uint16_t)x, (uint16_t)x);
        sendAddr(DISPLAY_SET_CURSOR_X, (uint16_t)0, (uint16_t)h - 1);
        sendCmd(DISPLAY_WRITE_PIXELS, pixelsBuffer, size);
    }

    return 0;
}

void SDL_ST7789_DestroyWindowFramebuffer(_THIS, SDL_Window *window)
{
    SDL_Surface *surface;

    surface = (SDL_Surface *)SDL_SetWindowData(window, ST7789_SURFACE, NULL);
    SDL_FreeSurface(surface);

    // printf("DestroyWindowFramebuffer\n");
}

void SDL_ST7789_VideoQuit(_THIS)
{
    printf("st7789 quit\n");
    sendCmdOnly(/*Display OFF*/ 0x28);
    clearGpio(GPIO_TFT_BACKLIGHT);
    DeinitSPI();
}

#endif /* SDL_VIDEO_DRIVER_ST7789 */

/* vi: set ts=4 sw=4 expandtab: */
