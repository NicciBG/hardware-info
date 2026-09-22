#ifndef GPU_NVIDIA_H
#define GPU_NVIDIA_H

/* NVIDIA GPU probing. Unlike AMD/Intel, the proprietary NVIDIA driver
   doesn't expose memory/clock state through sysfs — that's deliberately
   kept behind NVML. Rather than link NVML (a real SDK dependency, not
   just a header), identification comes from the driver's own
   /proc/driver/nvidia/gpus/<bus>/information file (always present
   whenever the proprietary driver is loaded, no extra package needed),
   and memory/clock figures are a best-effort read by invoking the
   `nvidia-smi` CLI tool that ships alongside the driver — see
   nvidia_smi_available below for when that wasn't possible.

   Only the proprietary driver is supported. The open-source `nouveau`
   driver doesn't populate /proc/driver/nvidia/ at all and isn't handled
   here — a real, documented gap, not an oversight.

   Verified against real hardware: this machine's RTX 3080. Only ever
   tested with a single NVIDIA GPU present — the multi-GPU matching
   between /proc entries and nvidia-smi rows (by UUID) is implemented but
   unverified against an actual multi-GPU machine. */

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;             /* "Model:" from /proc .../information */
    char *uuid;              /* "GPU UUID:" from /proc .../information */
    char *pci_bus_location;  /* e.g. "0000:01:00.0" */
    unsigned int pci_vendor_id;
    unsigned int pci_device_id;

    /* Best-effort, via nvidia-smi — see nvidia_smi_available on the
       aggregate struct. 0 / -1 when not available, not necessarily 0
       because that's the true value. */
    uint64_t vram_total_bytes;
    uint64_t vram_used_bytes;
    int core_clock_mhz;
    int memory_clock_mhz;
} GPU_NVIDIA_Device;

typedef struct {
    GPU_NVIDIA_Device *devices;
    int device_count;
    bool nvidia_smi_available; /* false if nvidia-smi couldn't be found/run
                                   — identification still works either
                                   way, only the memory/clock fields
                                   depend on this */
} GPU_NVIDIA_DATA;

DLL_EXPORT int get_nvidia_gpu_data(GPU_NVIDIA_DATA *data);
DLL_EXPORT void free_nvidia_gpu_data(GPU_NVIDIA_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* GPU_NVIDIA_H */

/*
0     Success (device_count may be 0 — not an error)
201   Null pointer passed to get_nvidia_gpu_data
203   Memory allocation failure
*/
