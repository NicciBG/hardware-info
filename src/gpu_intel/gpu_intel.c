#include "gpu_intel.h"
#include "gpu_intel_platform.h"

#include <stdlib.h>

int get_intel_gpu_data(GPU_INTEL_DATA *data) {
    if (!data) return 201;
    *data = (GPU_INTEL_DATA){0};
    return platform_get_intel_gpus(data);
}

void free_intel_gpu_data(GPU_INTEL_DATA *data) {
    if (!data) return;
    for (int i = 0; i < data->device_count; i++) {
        free(data->devices[i].name);
    }
    free(data->devices);
    *data = (GPU_INTEL_DATA){0};
}
