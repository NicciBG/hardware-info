#ifndef GPU_QUALCOMM_H
#define GPU_QUALCOMM_H

/* Qualcomm Adreno GPU probing, via the KGSL kernel driver's sysfs
   interface (/sys/class/kgsl/kgsl-3d*).

   IMPORTANT — confidence level, so this is used with the right
   expectations: this was written against the actual kgsl driver source
   (github.com/qualcomm-linux/kgsl, adreno_sysfs.c / kgsl_pwrctrl.c) to
   confirm attribute names and formats, which is firmer ground than a
   guess — but unlike AMD/Intel/NVIDIA, there is NO Qualcomm/Adreno
   hardware anywhere in this project's development environment, not even
   to confirm the zero-devices path finds the right absence. Every field
   here is genuinely unverified against a real device. Treat this as a
   first draft to be corrected by real test results, not as trustworthy
   as the other three vendor libraries.

   KGSL only ever exposes one GPU node in every case seen in the driver
   source (kgsl-3d0), but the scan below doesn't hardcode that — it
   matches any kgsl-3d* entry, in case that assumption turns out wrong. */

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;              /* gpu_model, e.g. "Adreno730v2" — raw driver
                                 string, not normalized */
    int current_clock_mhz;   /* gpuclk, converted from the driver's Hz */
    int max_clock_mhz;       /* max_gpuclk, converted from the driver's Hz */
    int busy_percent;        /* derived from gpubusy's two cycle counts;
                                 -1 if unavailable or the total was 0 */
} GPU_QUALCOMM_Device;

typedef struct {
    GPU_QUALCOMM_Device *devices;
    int device_count;
} GPU_QUALCOMM_DATA;

DLL_EXPORT int get_qualcomm_gpu_data(GPU_QUALCOMM_DATA *data);
DLL_EXPORT void free_qualcomm_gpu_data(GPU_QUALCOMM_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* GPU_QUALCOMM_H */

/*
0     Success (device_count may be 0 — not an error, and is what this
      returns on every machine this has actually run on so far)
201   Null pointer passed to get_qualcomm_gpu_data
203   Memory allocation failure
*/
