#pragma once

#ifndef _ST7789_SPI_H_
#define _ST7789_SPI_H_

#include <inttypes.h>
#include <unistd.h>   // usleep
#include <stdio.h>    // printf, stderr
#include <fcntl.h>    // open, O_RDWR, O_SYNC
#include <sys/mman.h> // mmap, munmap
#include <bcm_host.h> // bcm_host_get_peripheral_address, bcm_host_get_peripheral_size, bcm_host_get_sdram_address

#include "config.h"

typedef struct SPIRegisterFile
{
  uint32_t cs;   // SPI Master Control and Status register
  uint32_t fifo; // SPI Master TX and RX FIFOs
  uint32_t clk;  // SPI Master Clock Divider
  uint32_t dlen; // SPI Master Number of DMA Bytes to Write
} SPIRegisterFile;
extern volatile SPIRegisterFile *spi;

typedef struct GPIORegisterFile
{
  uint32_t gpfsel[6], reserved0; // GPIO Function Select registers, 3 bits per pin, 10 pins in an uint32_t
  uint32_t gpset[2], reserved1;  // GPIO Pin Output Set registers, write a 1 to bit at index I to set the pin at index I high
  uint32_t gpclr[2], reserved2;  // GPIO Pin Output Clear registers, write a 1 to bit at index I to set the pin at index I low
  uint32_t gplev[2];
} GPIORegisterFile;
extern volatile GPIORegisterFile *gpio;

#define SET_GPIO_MODE(pin, mode) gpio->gpfsel[(pin) / 10] = (gpio->gpfsel[(pin) / 10] & ~(0x7 << ((pin) % 10) * 3)) | ((mode) << ((pin) % 10) * 3)
#define GET_GPIO_MODE(pin) ((gpio->gpfsel[(pin) / 10] & (0x7 << ((pin) % 10) * 3)) >> (((pin) % 10) * 3))
#define GET_GPIO(pin) (gpio->gplev[0] & (1 << (pin))) // Pin must be (0-31)
#define SET_GPIO(pin) gpio->gpset[0] = 1 << (pin)     // Pin must be (0-31)
#define CLEAR_GPIO(pin) gpio->gpclr[0] = 1 << (pin)   // Pin must be (0-31)

void sendCmd(uint8_t cmd, uint8_t *payload, uint32_t payloadSize);
void sendCmdOnly(uint8_t cmd);
void sendCmdData(uint8_t cmd, uint8_t data);
int InitSPI();
void DeinitSPI();

#endif
