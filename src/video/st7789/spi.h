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

void sendCmd(uint8_t cmd, uint8_t *payload, uint32_t payloadSize);
void sendCmdOnly(uint8_t cmd);
void sendCmdData(uint8_t cmd, uint8_t data);
int InitSPI();
void DeinitSPI();

#endif
