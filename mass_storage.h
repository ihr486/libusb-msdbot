#ifndef MASS_STORAGE_H
#define MASS_STORAGE_H

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t msc_dir_t;

#define MSC_DIR_D2H (0x80)
#define MSC_DIR_H2D (0x00)

int mass_storage_send_command(const void *command_block, size_t command_block_length, msc_dir_t direction, size_t transfer_length);
int mass_storage_receive_status(size_t *residue);
int mass_storage_inquiry(void);

#endif
