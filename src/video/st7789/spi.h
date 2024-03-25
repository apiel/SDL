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

void sendCmd(uint8_t cmd, uint8_t *payload, uint32_t payloadSize);
void sendCmdOnly(uint8_t cmd);
void sendCmdData(uint8_t cmd, uint8_t data);
int InitSPI();
void DeinitSPI();

#endif
