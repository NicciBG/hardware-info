/* Standalone verification tool for libgpu_intel_*. See
   tools/gpu_amd_probe.c for the pattern this follows. */

#include "gpu_intel.h"

#include <stdio.h>
#include <time.h>

int main(void) {
    const char *out_path = "gpu_intel_probe_result.txt";
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Could not create %s in the current folder.\n", out_path);
        return 1;
    }

    time_t now = time(nullptr);
    fprintf(out, "Intel GPU probe result\n");
    fprintf(out, "generated: %s", ctime(&now));

    GPU_INTEL_DATA data;
    int rc = get_intel_gpu_data(&data);
    fprintf(out, "get_intel_gpu_data return code: %d\n\n", rc);

    if (rc != 0) {
        fprintf(out, "Call failed (see gpu_intel.h for what code %d means).\n", rc);
    } else {
        fprintf(out, "Intel GPUs found: %d\n\n", data.device_count);
        for (int i = 0; i < data.device_count; i++) {
            const GPU_INTEL_Device *d = &data.devices[i];
            fprintf(out, "Device %d:\n", i);
            fprintf(out, "  name:              %s\n", d->name);
            fprintf(out, "  pci_vendor_id:     0x%04x\n", d->pci_vendor_id);
            fprintf(out, "  pci_device_id:     0x%04x\n", d->pci_device_id);
            fprintf(out, "  max_freq_mhz:      %d%s\n", d->max_freq_mhz,
                    d->max_freq_mhz < 0 ? " (not available)" : "");
            fprintf(out, "  min_freq_mhz:      %d%s\n", d->min_freq_mhz,
                    d->min_freq_mhz < 0 ? " (not available)" : "");
            fprintf(out, "  current_freq_mhz:  %d%s\n\n", d->current_freq_mhz,
                    d->current_freq_mhz < 0 ? " (not available)" : "");
        }
        free_intel_gpu_data(&data);
    }

    fclose(out);

    printf("Done. Wrote %s in this folder.\n", out_path);
    printf("Please send that file back so the results can be checked against your actual GPU.\n");
    return 0;
}
