#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <sys/types.h>
#include <stdlib.h>

int usb_device_open(void);
void usb_device_close(void);

ssize_t usb_bulk_read(void *data, size_t length);
ssize_t usb_bulk_write(const void *data, size_t length);

#endif
