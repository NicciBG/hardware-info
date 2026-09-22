#include "gpu_qualcomm.h"
#include "gpu_qualcomm_platform.h"

#include <stdlib.h>

int get_qualcomm_gpu_data(GPU_QUALCOMM_DATA *data) {
    if (!data) return 201;
    *data = (GPU_QUALCOMM_DATA){0};
    return platform_get_qualcomm_gpus(data);
}

void free_qualcomm_gpu_data(GPU_QUALCOMM_DATA *data) {
    if (!data) return;
    for (int i = 0; i < data->device_count; i++) {
        free(data->devices[i].name);
    }
    free(data->devices);
    *data = (GPU_QUALCOMM_DATA){0};
}
