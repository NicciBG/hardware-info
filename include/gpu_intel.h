#ifndef GPU_INTEL_H
#define GPU_INTEL_H

/* Intel GPU probing — one unified library for both integrated and
   discrete parts, for the same reason as AMD: both go through the
   DRM/sysfs subsystem (i915 for current-generation hardware, xe for
   newer Battlemage-and-later parts), and the driver split tracks GPU
   generation rather than integrated-vs-discrete.

   Verified against real hardware: this machine's integrated UHD 770
   (i915 driver). No discrete Intel Arc card was available to verify the
   xe driver path, or the assumption below that VRAM isn't reported the
   same way on integrated parts. */

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name; /* i915 has no product-name sysfs file the way amdgpu
                    does, so this is always a fallback like
                    "Intel GPU (device 0xa780)" */
    unsigned int pci_vendor_id;
    unsigned int pci_device_id;

    int max_freq_mhz;     /* RP0 — max non-boost frequency */
    int min_freq_mhz;     /* RPn — min frequency */
    int current_freq_mhz; /* can legitimately read 0 when idle/power-gated,
                              that's not a read failure */

    /* No VRAM field: confirmed absent on this integrated GPU (no
       mem_info_*-style file anywhere under its sysfs tree). Discrete Arc
       cards may expose dedicated memory differently under the xe
       driver — untested, no such hardware available here. */
} GPU_INTEL_Device;

typedef struct {
    GPU_INTEL_Device *devices;
    int device_count;
} GPU_INTEL_DATA;

DLL_EXPORT int get_intel_gpu_data(GPU_INTEL_DATA *data);
DLL_EXPORT void free_intel_gpu_data(GPU_INTEL_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* GPU_INTEL_H */

/*
0     Success (device_count may be 0 — not an error)
201   Null pointer passed to get_intel_gpu_data
203   Memory allocation failure
*/
