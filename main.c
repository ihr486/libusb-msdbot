#include <stdio.h>
#include <stdlib.h>
#include "usb_device.h"
#include "mass_storage.h"

static void __attribute__((noreturn)) exit_with_error(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

int main(int argc, const char *argv[])
{
  usb_device_open();

  mass_storage_inquiry();

  usb_device_close();

  return EXIT_SUCCESS;
}
