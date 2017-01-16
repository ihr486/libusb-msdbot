#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

static void __attribute__((noreturn)) exit_with_error(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

int main(int argc, const char *argv[])
{
  libusb_context *usb = NULL;

  if (libusb_init(&usb))
  {
    exit_with_error("Failed to initialize libusb.");
  }

  libusb_set_debug(usb, LIBUSB_LOG_LEVEL_WARNING);

  libusb_device **device_list = NULL;
  ssize_t device_num = libusb_get_device_list(usb, &device_list);
  if (device_num < 0)
    exit_with_error("Error while enumerating devices.");
  else if (device_num == 0)
    exit_with_error("No devices found.");
  
  for (ssize_t i = 0; i < device_num; i++)
  {
    struct libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(device_list[i], &desc))
      exit_with_error("Failed to obtain descriptor.");

    printf("%d: %04X:%04X\n", i + 1, desc.idVendor, desc.idProduct);

    struct libusb_config_descriptor *conf_desc;
    if (libusb_get_active_config_descriptor(device_list[i], &conf_desc))
      exit_with_error("Failed to obtain configuration descriptor.");

    for (unsigned int j = 0; j < conf_desc->bNumInterfaces; j++)
    {
      const struct libusb_interface *interface = &conf_desc->interface[j];
      if (interface->num_altsetting >= 1)
      {
        const struct libusb_interface_descriptor *if_desc = &interface->altsetting[0];

        if (if_desc->bInterfaceClass == LIBUSB_CLASS_MASS_STORAGE && if_desc->bInterfaceSubClass == 0x06 && if_desc->bInterfaceProtocol == 0x50)
        {
          printf("\tInterface #%d: Mass storage, SCSI transparent command set, Bulk-only transport\n", j);
        }
      }
    }

    libusb_free_config_descriptor(conf_desc);
  }

  printf("Select device [1-%d]: ", device_num);

  int device_index;
  scanf("%d", &device_index);

  if (device_index < 1 || device_num < device_index)
    exit_with_error("Device index is out of range.");

  printf("Opening device #%d...\n", device_index);

  libusb_device_handle *handle;

  if (libusb_open(device_list[device_index - 1], &handle))
    exit_with_error("Failed to open the device.");
  
  libusb_free_device_list(device_list, 1);

  libusb_set_auto_detach_kernel_driver(handle, 1);

  if (libusb_claim_interface(handle, 0))
    exit_with_error("Failed to claim interface #0.");

  libusb_release_interface(handle, 0);

  libusb_close(handle);

  libusb_exit(usb);

  return EXIT_SUCCESS;
}
