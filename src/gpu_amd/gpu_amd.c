#include "gpu_amd.h"
#include "gpu_amd_platform.h"

#include <stdlib.h>

int get_amd_gpu_data(GPU_AMD_DATA *data) {
    if (!data) return 201;
    *data = (GPU_AMD_DATA){0};
    return platform_get_amd_gpus(data);
}

void free_amd_gpu_data(GPU_AMD_DATA *data) {
    if (!data) return;
    for (int i = 0; i < data->device_count; i++) {
        free(data->devices[i].name);
    }
    free(data->devices);
    *data = (GPU_AMD_DATA){0};
}
