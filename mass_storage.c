#include <stdio.h>
#include <string.h>

#include "mass_storage.h"
#include "usb_device.h"

static inline void write_32LE(void *p, size_t offset, uint32_t value)
{
  ((uint8_t *)p)[offset] = (value) & 0x000000FF;
  ((uint8_t *)p)[offset + 1] = (value >> 8) & 0x000000FF;
  ((uint8_t *)p)[offset + 2] = (value >> 16) & 0x000000FF;
  ((uint8_t *)p)[offset + 3] = (value >> 24) & 0x000000FF;
}

static inline uint32_t read_32LE(const void *p, size_t offset)
{
  uint32_t value;
  value = ((uint8_t *)p)[offset];
  value |= ((uint32_t)((uint8_t *)p)[offset + 1]) << 8;
  value |= ((uint32_t)((uint8_t *)p)[offset + 2]) << 16;
  value |= ((uint32_t)((uint8_t *)p)[offset + 3]) << 24;
  return value;
}

static inline uint32_t read_32BE(const void *p, size_t offset)
{
  uint32_t value;
  value = ((uint32_t)((uint8_t *)p)[offset]) << 24;
  value |= ((uint32_t)((uint8_t *)p)[offset + 1]) << 16;
  value |= ((uint32_t)((uint8_t *)p)[offset + 2]) << 8;
  value |= ((uint8_t *)p)[offset + 3];
  return value;
}

static inline void write_32BE(void *p, size_t offset, uint32_t value)
{
  ((uint8_t *)p)[offset] = (value >> 24) & 0x000000FF;
  ((uint8_t *)p)[offset + 1] = (value >> 16) & 0x000000FF;
  ((uint8_t *)p)[offset + 2] = (value >> 8) & 0x000000FF;
  ((uint8_t *)p)[offset + 3] = (value) & 0x000000FF;
}

static inline void write_16BE(void *p, size_t offset, uint16_t value)
{
  ((uint8_t *)p)[offset] = (value >> 8) & 0x000000FF;
  ((uint8_t *)p)[offset + 1] = (value) & 0x000000FF;
}

int mass_storage_send_command(const void *command_block, size_t command_block_length, msc_dir_t direction, size_t transfer_length)
{
  uint8_t cbw[31];
  memset(cbw, 0, 31);
  memcpy(cbw + 15, command_block, command_block_length);
  write_32LE(cbw, 0, 0x43425355);
  write_32LE(cbw, 4, 0x00000000);
  write_32LE(cbw, 8, transfer_length);
  cbw[12] = direction;
  cbw[13] = 0;
  cbw[14] = command_block_length;
  return usb_bulk_write(cbw, 31);
}

int mass_storage_receive_status(size_t *residue)
{
  uint8_t csw[13];
  int result = usb_bulk_read(csw, 13);
  if (result < 0) return result;
  if (residue != NULL) *residue = read_32LE(csw, 8);
  return csw[12];
}

int mass_storage_inquiry(void)
{
  uint8_t cb[6];
  memset(cb, 0, 6);
  cb[0] = 0x12;
  cb[4] = 36;
  int result = mass_storage_send_command(cb, 6, MSC_DIR_D2H, 36);
  if (result < 0) return result;
  uint8_t data[36];
  result = usb_bulk_read(data, 36);
  if (result < 0) return result;
  if (result != 36)
  {
    printf("INQUIRY command failed.\n");
  }
  else
  {
    char vendor[9], product[17], revision[5];
    memcpy(vendor, data + 8, 8);
    memcpy(product, data + 16, 16);
    memcpy(revision, data + 32, 4);
    vendor[8] = '\0';
    product[16] = '\0';
    revision[4] = '\0';
    printf("Vendor = \"%s\", Product = \"%s\", Revision = \"%s\"\n", vendor, product, revision);
  }
  return mass_storage_receive_status(NULL);
}

int mass_storage_read(void *data, uint32_t sector, uint16_t count)
{
  uint8_t cb[10];
  memset(cb, 0, 10);
  cb[0] = 0x28;
  write_32BE(cb, 2, sector);
  write_16BE(cb, 7, count);
  int result = mass_storage_send_command(cb, 10, MSC_DIR_D2H, count * 512);
  if (result < 0) return result;
  result = usb_bulk_read(data, count * 512);
  if (result < 0) return result;
  return mass_storage_receive_status(NULL);
}

int mass_storage_write(const void *data, uint32_t sector, uint16_t count)
{
  uint8_t cb[10];
  memset(cb, 0, 10);
  cb[0] = 0x2A;
  write_32BE(cb, 2, sector);
  write_16BE(cb, 7, count);
  int result = mass_storage_send_command(cb, 10, MSC_DIR_H2D, count * 512);
  if (result < 0) return result;
  result = usb_bulk_write(data, count * 512);
  if (result < 0) return result;
  return mass_storage_receive_status(NULL);
}

int mass_storage_read_capacity(void)
{
  uint8_t cb[10];
  memset(cb, 0, 10);
  cb[0] = 0x25;
  int result = mass_storage_send_command(cb, 10, MSC_DIR_D2H, 8);
  if (result < 0) return result;
  uint8_t data[8];
  result = usb_bulk_read(data, 8);
  if (result < 0) return result;
  printf("%u sectors (%u bytes each)\n", read_32BE(data, 0), read_32BE(data, 4));
  return mass_storage_receive_status(NULL);
}

int mass_storage_test_unit_ready(void)
{
  uint8_t cb[6];
  memset(cb, 0, 6);
  int result = mass_storage_send_command(cb, 6, MSC_DIR_D2H, 0);
  if (result < 0) return result;
  result = mass_storage_receive_status(NULL);
  if (result == 0)
    printf("Device ready\n");
  else
    printf("Device not ready\n");
  return result;
}
