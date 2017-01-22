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
  mass_storage_read_capacity();
  mass_storage_test_unit_ready();

  uint8_t mbr[512];

  mass_storage_read(mbr, 0, 1);

  FILE *fp = fopen("mbr.bin", "wb");
  fwrite(mbr, 1, 512, fp);
  fclose(fp);

  usb_device_close();

  return EXIT_SUCCESS;
}
