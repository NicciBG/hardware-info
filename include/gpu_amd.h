#ifndef GPU_AMD_H
#define GPU_AMD_H

/* AMD GPU probing — one unified library for both integrated (APU) and
   discrete Radeon parts, since both are handled by the same amdgpu kernel
   driver through the same sysfs API on Linux (confirmed against kernel
   docs: docs.kernel.org/gpu/amdgpu). Values differ in meaning between
   the two (an APU's "VRAM" is a BIOS-carved-out slice of system RAM, not
   dedicated memory) but the query mechanism doesn't, so there's no d/i
   split here.

   NOTE: written against documented amdgpu sysfs conventions but NOT
   verified against real AMD hardware — no AMD GPU exists on the machine
   this was developed on (confirmed via lspci: only Intel iGPU + NVIDIA
   discrete present). See tools/gpu_amd_probe.c for the standalone
   verification executable meant to be run on a real AMD machine. */

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name; /* product_name from sysfs if present, else a PCI-ID-based
                    fallback like "AMD GPU (device 0x73bf)" */
    unsigned int pci_vendor_id;
    unsigned int pci_device_id;

    uint64_t vram_total_bytes;
    uint64_t vram_used_bytes;
    uint64_t gtt_total_bytes; /* graphics translation table memory —
                                  particularly relevant on APUs */

    int engine_clock_mhz; /* current sclk, best-effort from pp_dpm_sclk */
    int memory_clock_mhz; /* current mclk, best-effort from pp_dpm_mclk */
} GPU_AMD_Device;

typedef struct {
    GPU_AMD_Device *devices;
    int device_count;
} GPU_AMD_DATA;

/* Known gaps, not attempted here: compute-unit/shader-core count and
   feature flags (ray tracing support, etc.) would need either a large,
   constantly-stale hardcoded PCI-ID lookup table or an external SDK
   (ROCm/Vulkan), both against this project's constraints. Also not
   attempted: classifying a device as integrated vs discrete — the only
   sysfs-visible heuristic (small vram_total on APUs) is unreliable
   enough that reporting the raw numbers and letting the caller judge
   seemed more honest than a guess presented as fact. */

DLL_EXPORT int get_amd_gpu_data(GPU_AMD_DATA *data);
DLL_EXPORT void free_amd_gpu_data(GPU_AMD_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* GPU_AMD_H */

/*
0     Success (device_count may be 0 if no AMD GPU is present — that's
      not an error)
201   Null pointer passed to get_amd_gpu_data
203   Memory allocation failure
*/
