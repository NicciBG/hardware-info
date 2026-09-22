#include "gpu_nvidia.h"
#include "gpu_nvidia_platform.h"

#include <stdlib.h>

int get_nvidia_gpu_data(GPU_NVIDIA_DATA *data) {
    if (!data) return 201;
    *data = (GPU_NVIDIA_DATA){0};
    return platform_get_nvidia_gpus(data);
}

void free_nvidia_gpu_data(GPU_NVIDIA_DATA *data) {
    if (!data) return;
    for (int i = 0; i < data->device_count; i++) {
        free(data->devices[i].name);
        free(data->devices[i].uuid);
        free(data->devices[i].pci_bus_location);
    }
    free(data->devices);
    *data = (GPU_NVIDIA_DATA){0};
}
