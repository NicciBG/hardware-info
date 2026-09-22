/* Common logic: just orchestration and teardown. Unlike CPU detection,
   there's no OS-independent mechanism for RAM/storage the way CPUID is
   for instruction-set features, so almost everything else lives in the
   platform files. */

#include "hardware_info.h"
#include "hardware_info_platform.h"

#include <stdlib.h>

int get_hardware_data(HARDWARE_DATA *data) {
    if (!data) return 201;
    *data = (HARDWARE_DATA){0};

    int rc = platform_get_ram_info(data);
    if (rc != 0) return rc;

    rc = platform_get_storage_devices(data);
    if (rc != 0) {
        free_hardware_data(data);
        return rc;
    }

    return 0;
}

void free_hardware_data(HARDWARE_DATA *data) {
    if (!data) return;

    for (int i = 0; i < data->storage_device_count; i++) {
        free(data->storage_devices[i].device_name);
        free(data->storage_devices[i].model);
        free(data->storage_devices[i].interface_driver);
    }
    free(data->storage_devices);

    *data = (HARDWARE_DATA){0};
}
