#ifndef HARDWARE_INFO_PLATFORM_H
#define HARDWARE_INFO_PLATFORM_H

/* Internal seam between the common logic (hardware_info.c) and each
   platform's implementation. Not part of the public API. */

#include "hardware_info.h"

int platform_get_ram_info(HARDWARE_DATA *data);
int platform_get_storage_devices(HARDWARE_DATA *data);

#endif /* HARDWARE_INFO_PLATFORM_H */
