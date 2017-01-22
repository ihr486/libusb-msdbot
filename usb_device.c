#include <stdio.h>
#include <libusb.h>

static libusb_context *usb = NULL;
static struct libusb_device_handle *device = NULL;
static uint8_t endpoint_d2h = 0, endpoint_h2d = 0;
static uint16_t payload_d2h = 0, payload_h2d = 0;

#define LIBUSB_CHECK(action) \
do { \
  int ret = (action); \
  if (ret != LIBUSB_SUCCESS) \
  { \
    fprintf(stderr, "%s\n", libusb_strerror(ret)); \
    return -1; \
  } \
} while(0)

int usb_device_open(void)
{
  LIBUSB_CHECK(libusb_init(&usb));

  libusb_set_debug(usb, LIBUSB_LOG_LEVEL_WARNING);

  libusb_device **device_list = NULL;
  ssize_t device_num = libusb_get_device_list(usb, &device_list);
  if (device_num <= 0)
  {
    fprintf(stderr, "No device found on the bus.\n");
    return -1;
  }

  for (ssize_t i = 0; i < device_num; i++)
  {
    struct libusb_device_descriptor desc;
    LIBUSB_CHECK(libusb_get_device_descriptor(device_list[i], &desc));

    printf("%d: %04X:%04X\n", i + 1, desc.idVendor, desc.idProduct);

    struct libusb_config_descriptor *conf_desc;
    LIBUSB_CHECK(libusb_get_active_config_descriptor(device_list[i], &conf_desc));

    for (unsigned int j = 0; j < conf_desc->bNumInterfaces; j++)
    {
      const struct libusb_interface *interface = &conf_desc->interface[j];
      if (interface->num_altsetting >= 1)
      {
        const struct libusb_interface_descriptor *if_desc = &interface->altsetting[0];

        if (if_desc->bInterfaceClass == LIBUSB_CLASS_MASS_STORAGE && if_desc->bInterfaceSubClass == 0x06 && if_desc->bInterfaceProtocol == 0x50)
        {
          printf("\tInterface #%d: Mass storage, SCSI transparent command set, Bulk-only transport\n", j);

          for (unsigned int k = 0; k < if_desc->bNumEndpoints; k++)
          {
            const struct libusb_endpoint_descriptor *ep_desc = &if_desc->endpoint[k];

            if ((ep_desc->bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_BULK)
            {
              if (ep_desc->bEndpointAddress & 0x80)
              {
                printf("\tEndpoint 0x%02X: Bulk D2H (Payload = %u bytes)\n", ep_desc->bEndpointAddress, ep_desc->wMaxPacketSize);
                endpoint_d2h = ep_desc->bEndpointAddress;
                payload_d2h = ep_desc->wMaxPacketSize;
              }
              else
              {
                printf("\tEndpoint 0x%02X: Bulk H2D (Payload = %u bytes)\n", ep_desc->bEndpointAddress, ep_desc->wMaxPacketSize);
                endpoint_h2d = ep_desc->bEndpointAddress;
                payload_h2d = ep_desc->wMaxPacketSize;
              }
            }
          }

          if (endpoint_d2h == 0 || endpoint_h2d == 0)
          {
            fprintf(stderr, "Interface is not complete.\n");
            return -1;
          }

          LIBUSB_CHECK(libusb_open(device_list[i], &device));
          libusb_set_auto_detach_kernel_driver(device, 1);
          LIBUSB_CHECK(libusb_claim_interface(device, 0));
          libusb_free_device_list(device_list, 1);
          return 0;
        }
      }
    }

    libusb_free_config_descriptor(conf_desc);
  }
  libusb_free_device_list(device_list, 1);
  fprintf(stderr, "No target device found.\n");
  return -1;
}

void usb_device_close(void)
{
  if (device != NULL)
    libusb_close(device);
  if (usb != NULL)
    libusb_exit(usb);
}

ssize_t usb_bulk_read(void *data, size_t length)
{
  int transferred;
  LIBUSB_CHECK(libusb_bulk_transfer(device, endpoint_d2h, (unsigned char *)data, length, &transferred, 0));
  return transferred;
}

ssize_t usb_bulk_write(const void *data, size_t length)
{
  int transferred;
  LIBUSB_CHECK(libusb_bulk_transfer(device, endpoint_h2d, (unsigned char *)data, length, &transferred, 0));
  return transferred;
}
