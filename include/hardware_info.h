#ifndef HARDWARE_INFO_H
#define HARDWARE_INFO_H

/* RAM and storage-device probing — the non-CPU half of hardware
   detection (see cpu_info.h for the CPU/CPUID half; GPU is intentionally
   out of scope here). Same file-split convention: this header plus
   hardware_info.c hold what's common, libhardware_info_linux.c /
   hardware_info_windows.c / libhardware_info_macos.c hold what's
   platform-specific — for RAM/storage that's nearly everything, since
   unlike CPUID there's no OS-independent mechanism to query either. */

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

typedef enum {
    STORAGE_MEDIA_HDD,     /* rotational */
    STORAGE_MEDIA_SSD,     /* non-rotational */
    STORAGE_MEDIA_UNKNOWN,
} StorageMedia;

typedef enum {
    STORAGE_INTERFACE_NVME,
    STORAGE_INTERFACE_SATA,
    STORAGE_INTERFACE_PATA,
    STORAGE_INTERFACE_SAS,
    STORAGE_INTERFACE_USB,
    STORAGE_INTERFACE_MMC, /* SD card / eMMC */
    STORAGE_INTERFACE_VIRTIO, /* virtual disk inside a VM */
    STORAGE_INTERFACE_FIREWIRE,
    STORAGE_INTERFACE_UNKNOWN, /* see interface_driver for what it actually was */
} StorageInterface;

typedef struct {
    char *device_name; /* e.g. "nvme0n1", "sda" */
    char *model;       /* e.g. "Samsung SSD 980 PRO 1TB"; may be empty */
    uint64_t size_bytes;
    StorageMedia media;
    StorageInterface interface_type;
    char *interface_driver; /* raw driver/proc_name backing interface_type,
                                e.g. "ahci", "usb-storage", "nvme" — always
                                populated, most useful when interface_type
                                is STORAGE_INTERFACE_UNKNOWN */
} StorageDevice;

typedef struct {
    uint64_t total_ram_kib;
    uint64_t available_ram_kib; /* MemAvailable-equivalent: what's usable
                                    without swapping, not just MemFree */

    StorageDevice *storage_devices;
    int storage_device_count;
} HARDWARE_DATA;

/* Allocate and zero a HARDWARE_DATA before calling. Free with
   free_hardware_data(), not by hand, for the same cross-shared-library
   allocator reason as free_cpu_data(). */
DLL_EXPORT int get_hardware_data(HARDWARE_DATA *data);
DLL_EXPORT void free_hardware_data(HARDWARE_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_INFO_H */

/*
0     Success
201   Null pointer passed to get_hardware_data
202   Failed to open a required system info source
203   Memory allocation failure
204-207  Platform-specific API failures
*/
