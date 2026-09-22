/* Standalone verification tool for libgpu_qualcomm_*. See
   tools/gpu_amd_probe.c for the pattern this follows. */

#include "gpu_qualcomm.h"

#include <stdio.h>
#include <time.h>

int main(void) {
    const char *out_path = "gpu_qualcomm_probe_result.txt";
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Could not create %s in the current folder.\n", out_path);
        return 1;
    }

    time_t now = time(nullptr);
    fprintf(out, "Qualcomm Adreno GPU probe result\n");
    fprintf(out, "generated: %s", ctime(&now));

    GPU_QUALCOMM_DATA data;
    int rc = get_qualcomm_gpu_data(&data);
    fprintf(out, "get_qualcomm_gpu_data return code: %d\n\n", rc);

    if (rc != 0) {
        fprintf(out, "Call failed (see gpu_qualcomm.h for what code %d means).\n", rc);
    } else {
        fprintf(out, "Adreno GPUs found: %d\n\n", data.device_count);
        for (int i = 0; i < data.device_count; i++) {
            const GPU_QUALCOMM_Device *d = &data.devices[i];
            fprintf(out, "Device %d:\n", i);
            fprintf(out, "  name:               %s\n", d->name);
            fprintf(out, "  current_clock_mhz:  %d%s\n", d->current_clock_mhz,
                    d->current_clock_mhz < 0 ? " (not available)" : "");
            fprintf(out, "  max_clock_mhz:      %d%s\n", d->max_clock_mhz,
                    d->max_clock_mhz < 0 ? " (not available)" : "");
            fprintf(out, "  busy_percent:       %d%s\n\n", d->busy_percent,
                    d->busy_percent < 0 ? " (not available)" : "");
        }
        free_qualcomm_gpu_data(&data);
    }

    fclose(out);

    printf("Done. Wrote %s in this folder.\n", out_path);
    printf("Please send that file back so the results can be checked against your actual GPU.\n");
    return 0;
}
