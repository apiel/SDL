#pragma once

#include <inttypes.h>
#include <unistd.h>   // usleep
#include <stdio.h>    // printf, stderr
#include <fcntl.h>    // open, O_RDWR, O_SYNC
#include <sys/mman.h> // mmap, munmap
#include <bcm_host.h> // bcm_host_get_peripheral_address, bcm_host_get_peripheral_size, bcm_host_get_sdram_address

#include "config.h"

#define BCM2835_GPIO_BASE 0x200000 // Address to GPIO register file
#define BCM2835_SPI0_BASE 0x204000 // Address to SPI0 register file
#define BCM2835_TIMER_BASE 0x3000  // Address to System Timer register file

#define BCM2835_SPI0_CS_RXF 0x00100000      // Receive FIFO is full
#define BCM2835_SPI0_CS_RXR 0x00080000      // FIFO needs reading
#define BCM2835_SPI0_CS_TXD 0x00040000      // TXD TX FIFO can accept Data
#define BCM2835_SPI0_CS_RXD 0x00020000      // RXD RX FIFO contains Data
#define BCM2835_SPI0_CS_DONE 0x00010000     // Done transfer Done
#define BCM2835_SPI0_CS_ADCS 0x00000800     // Automatically Deassert Chip Select
#define BCM2835_SPI0_CS_INTR 0x00000400     // Fire interrupts on RXR?
#define BCM2835_SPI0_CS_INTD 0x00000200     // Fire interrupts on DONE?
#define BCM2835_SPI0_CS_DMAEN 0x00000100    // Enable DMA transfers?
#define BCM2835_SPI0_CS_TA 0x00000080       // Transfer Active
#define BCM2835_SPI0_CS_CLEAR 0x00000030    // Clear FIFO Clear RX and TX
#define BCM2835_SPI0_CS_CLEAR_RX 0x00000020 // Clear FIFO Clear RX
#define BCM2835_SPI0_CS_CLEAR_TX 0x00000010 // Clear FIFO Clear TX
#define BCM2835_SPI0_CS_CPOL 0x00000008     // Clock Polarity
#define BCM2835_SPI0_CS_CPHA 0x00000004     // Clock Phase
#define BCM2835_SPI0_CS_CS 0x00000003       // Chip Select

#define BCM2835_SPI0_CS_RXF_SHIFT 20
#define BCM2835_SPI0_CS_RXR_SHIFT 19
#define BCM2835_SPI0_CS_TXD_SHIFT 18
#define BCM2835_SPI0_CS_RXD_SHIFT 17
#define BCM2835_SPI0_CS_DONE_SHIFT 16
#define BCM2835_SPI0_CS_ADCS_SHIFT 11
#define BCM2835_SPI0_CS_INTR_SHIFT 10
#define BCM2835_SPI0_CS_INTD_SHIFT 9
#define BCM2835_SPI0_CS_DMAEN_SHIFT 8
#define BCM2835_SPI0_CS_TA_SHIFT 7
#define BCM2835_SPI0_CS_CLEAR_RX_SHIFT 5
#define BCM2835_SPI0_CS_CLEAR_TX_SHIFT 4
#define BCM2835_SPI0_CS_CPOL_SHIFT 3
#define BCM2835_SPI0_CS_CPHA_SHIFT 2
#define BCM2835_SPI0_CS_CS_SHIFT 0

#define GPIO_SPI0_MOSI 10 // Pin P1-19, MOSI when SPI0 in use
#define GPIO_SPI0_MISO 9  // Pin P1-21, MISO when SPI0 in use
#define GPIO_SPI0_CLK 11  // Pin P1-23, CLK when SPI0 in use
#define GPIO_SPI0_CE0 8   // Pin P1-24, CE0 when SPI0 in use
#define GPIO_SPI0_CE1 7   // Pin P1-26, CE1 when SPI0 in use

#define DISPLAY_SPI_DRIVE_SETTINGS (1 | BCM2835_SPI0_CS_CPOL | BCM2835_SPI0_CS_CPHA)

extern volatile void *bcm2835;

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

typedef struct SPIRegisterFile
{
  uint32_t cs;   // SPI Master Control and Status register
  uint32_t fifo; // SPI Master TX and RX FIFOs
  uint32_t clk;  // SPI Master Clock Divider
  uint32_t dlen; // SPI Master Number of DMA Bytes to Write
} SPIRegisterFile;
extern volatile SPIRegisterFile *spi;

int mem_fd = -1;
volatile void *bcm2835 = 0;
volatile GPIORegisterFile *gpio = 0;
volatile SPIRegisterFile *spi = 0;

// Errata to BCM2835 behavior: documentation states that the SPI0 DLEN register is only used for DMA. However, even when DMA is not being utilized, setting it from
// a value != 0 or 1 gets rid of an excess idle clock cycle that is present when transmitting each byte. (by default in Polled SPI Mode each 8 bits transfer in 9 clocks)
// With DLEN=2 each byte is clocked to the bus in 8 cycles, observed to improve max throughput from 56.8mbps to 63.3mbps (+11.4%, quite close to the theoretical +12.5%)
// https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=181154
#define UNLOCK_FAST_8_CLOCKS_SPI() (spi->dlen = 2)

void WaitForPolledSPITransferToFinish()
{
  uint32_t cs;
  while (!(((cs = spi->cs) ^ BCM2835_SPI0_CS_TA) & (BCM2835_SPI0_CS_DONE | BCM2835_SPI0_CS_TA))) // While TA=1 and DONE=0
    if ((cs & (BCM2835_SPI0_CS_RXR | BCM2835_SPI0_CS_RXF)))
      spi->cs = BCM2835_SPI0_CS_CLEAR_RX | BCM2835_SPI0_CS_TA | DISPLAY_SPI_DRIVE_SETTINGS;

  if ((cs & BCM2835_SPI0_CS_RXD))
    spi->cs = BCM2835_SPI0_CS_CLEAR_RX | BCM2835_SPI0_CS_TA | DISPLAY_SPI_DRIVE_SETTINGS;
}

void sendCmd(uint8_t cmd, uint8_t *payload, uint32_t payloadSize)
{
  int8_t *tStart;
  uint8_t *tEnd;
  uint8_t *tPrefillEnd;
  uint32_t cs;

  // WaitForPolledSPITransferToFinish();

  spi->cs = BCM2835_SPI0_CS_TA | DISPLAY_SPI_DRIVE_SETTINGS; // Spi begins transfer

  // An SPI transfer to the display always starts with one control (command) byte, followed by N data bytes.
  CLEAR_GPIO(GPIO_TFT_DATA_CONTROL);

  spi->fifo = cmd;
  while (!(spi->cs & (BCM2835_SPI0_CS_RXD | BCM2835_SPI0_CS_DONE))) /*nop*/
    ;

  if (payloadSize > 0)
  {
    SET_GPIO(GPIO_TFT_DATA_CONTROL);

    tStart = payload;
    tEnd = payload + payloadSize;
    tPrefillEnd = tStart + (payloadSize > 15 ? 15 : payloadSize);

    while (tStart < tPrefillEnd)
      spi->fifo = *tStart++;
    while (tStart < tEnd)
    {
      cs = spi->cs;
      if ((cs & BCM2835_SPI0_CS_TXD))
        spi->fifo = *tStart++;
      if ((cs & (BCM2835_SPI0_CS_RXR | BCM2835_SPI0_CS_RXF)))
        spi->cs = BCM2835_SPI0_CS_CLEAR_RX | BCM2835_SPI0_CS_TA | DISPLAY_SPI_DRIVE_SETTINGS;
    }
  }

  WaitForPolledSPITransferToFinish();
}

void sendCmdOnly(uint8_t cmd)
{
  sendCmd(cmd, NULL, 0);
}

void sendCmdData(uint8_t cmd, uint8_t data)
{
  sendCmd(cmd, &data, 1);
}

int InitSPI()
{
  // Memory map GPIO and SPI peripherals for direct access
  mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (mem_fd < 0)
  {
    fprintf(stderr, "can't open /dev/mem (run as sudo)\n");
    return -1;
  }

  printf("bcm_host_get_peripheral_address %p, bcm_host_get_peripheral_size: %u, bcm_host_get_sdram_address: %p\n", bcm_host_get_peripheral_address(), bcm_host_get_peripheral_size(), bcm_host_get_sdram_address());
  bcm2835 = mmap(NULL, bcm_host_get_peripheral_size(), (PROT_READ | PROT_WRITE), MAP_SHARED, mem_fd, bcm_host_get_peripheral_address());
  if (bcm2835 == MAP_FAILED)
  {
    fprintf(stderr, "mapping /dev/mem failed\n");
    return -1;
  }
  spi = (volatile SPIRegisterFile *)((uintptr_t)bcm2835 + BCM2835_SPI0_BASE);
  gpio = (volatile GPIORegisterFile *)((uintptr_t)bcm2835 + BCM2835_GPIO_BASE);

  SET_GPIO_MODE(GPIO_TFT_DATA_CONTROL, 0x01); // Data/Control pin to output (0x01)
  SET_GPIO_MODE(GPIO_SPI0_MOSI, 0x04);
  SET_GPIO_MODE(GPIO_SPI0_CLK, 0x04);

  spi->cs = BCM2835_SPI0_CS_CLEAR | DISPLAY_SPI_DRIVE_SETTINGS; // Initialize the Control and Status register to defaults: CS=0 (Chip Select), CPHA=0 (Clock Phase), CPOL=0 (Clock Polarity), CSPOL=0 (Chip Select Polarity), TA=0 (Transfer not active), and reset TX and RX queues.
  spi->clk = SPI_BUS_CLOCK_DIVISOR;                             // Clock Divider determines SPI bus speed, resulting speed=256MHz/clk

  // Enable fast 8 clocks per byte transfer mode, instead of slower 9 clocks per byte.
  UNLOCK_FAST_8_CLOCKS_SPI();

  return 0;
}

void DeinitSPI()
{
  spi->cs = BCM2835_SPI0_CS_CLEAR | DISPLAY_SPI_DRIVE_SETTINGS;

  SET_GPIO_MODE(GPIO_TFT_DATA_CONTROL, 0);

  SET_GPIO_MODE(GPIO_SPI0_CE1, 0);
  SET_GPIO_MODE(GPIO_SPI0_CE0, 0);
  SET_GPIO_MODE(GPIO_SPI0_MISO, 0);
  SET_GPIO_MODE(GPIO_SPI0_MOSI, 0);
  SET_GPIO_MODE(GPIO_SPI0_CLK, 0);

  if (bcm2835)
  {
    munmap((void *)bcm2835, bcm_host_get_peripheral_size());
    bcm2835 = 0;
  }

  if (mem_fd >= 0)
  {
    close(mem_fd);
    mem_fd = -1;
  }
}
