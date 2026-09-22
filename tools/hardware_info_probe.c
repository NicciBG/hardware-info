/* Standalone verification tool for libhardware_info_*. Same pattern as
   tools/gpu_amd_probe.c — particularly useful here for storage
   interfaces that were never available to test against directly
   (SAS, USB, PATA, MMC/eMMC — see hardware_info.h for exactly which
   interface classification was verified and which wasn't). */

#include "hardware_info.h"

#include <stdio.h>
#include <time.h>

static const char *media_name(StorageMedia m) {
    switch (m) {
        case STORAGE_MEDIA_HDD: return "HDD";
        case STORAGE_MEDIA_SSD: return "SSD";
        default: return "unknown";
    }
}

static const char *interface_name(StorageInterface i) {
    switch (i) {
        case STORAGE_INTERFACE_NVME: return "NVMe";
        case STORAGE_INTERFACE_SATA: return "SATA";
        case STORAGE_INTERFACE_PATA: return "PATA";
        case STORAGE_INTERFACE_SAS: return "SAS";
        case STORAGE_INTERFACE_USB: return "USB";
        case STORAGE_INTERFACE_MMC: return "MMC/eMMC";
        case STORAGE_INTERFACE_VIRTIO: return "virtio";
        case STORAGE_INTERFACE_FIREWIRE: return "FireWire";
        default: return "unknown";
    }
}

int main(void) {
    const char *out_path = "hardware_info_probe_result.txt";
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Could not create %s in the current folder.\n", out_path);
        return 1;
    }

    time_t now = time(nullptr);
    fprintf(out, "Hardware (RAM/storage) probe result\n");
    fprintf(out, "generated: %s", ctime(&now));

    HARDWARE_DATA data;
    int rc = get_hardware_data(&data);
    fprintf(out, "get_hardware_data return code: %d\n\n", rc);

    if (rc != 0) {
        fprintf(out, "Call failed (see hardware_info.h for what code %d means).\n", rc);
        fclose(out);
        printf("Done. Wrote %s in this folder (call failed, see the file).\n", out_path);
        return 0;
    }

    fprintf(out, "total_ram_kib: %llu (%.2f GB)\n", (unsigned long long)data.total_ram_kib,
            (double)data.total_ram_kib / 1e6);
    fprintf(out, "available_ram_kib: %llu (%.2f GB)\n\n",
            (unsigned long long)data.available_ram_kib,
            (double)data.available_ram_kib / 1e6);

    fprintf(out, "Storage devices found: %d\n\n", data.storage_device_count);
    for (int i = 0; i < data.storage_device_count; i++) {
        StorageDevice *d = &data.storage_devices[i];
        fprintf(out, "Device %d:\n", i);
        fprintf(out, "  device_name:       %s\n", d->device_name);
        fprintf(out, "  model:             %s\n", d->model);
        fprintf(out, "  size_bytes:        %llu (%.2f GB)\n",
                (unsigned long long)d->size_bytes, (double)d->size_bytes / 1e9);
        fprintf(out, "  media:             %s\n", media_name(d->media));
        fprintf(out, "  interface_type:    %s\n", interface_name(d->interface_type));
        fprintf(out, "  interface_driver:  %s\n\n", d->interface_driver);
    }

    free_hardware_data(&data);
    fclose(out);

    printf("Done. Wrote %s in this folder.\n", out_path);
    printf("Please send that file back so the results can be checked against your actual hardware.\n");
    return 0;
}
