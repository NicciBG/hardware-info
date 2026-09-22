/* Standalone verification tool for libgpu_amd_*. Meant to be shipped next
   to the built shared library and run on a real AMD machine by someone
   who isn't expected to build anything themselves — it writes a plain
   text file with everything the library detected, to be sent back and
   checked against the tester's actual hardware. */

#include "gpu_amd.h"

#include <stdio.h>
#include <time.h>

int main(void) {
    const char *out_path = "gpu_amd_probe_result.txt";
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Could not create %s in the current folder.\n", out_path);
        return 1;
    }

    time_t now = time(nullptr);
    fprintf(out, "AMD GPU probe result\n");
    fprintf(out, "generated: %s", ctime(&now));

    GPU_AMD_DATA data;
    int rc = get_amd_gpu_data(&data);
    fprintf(out, "get_amd_gpu_data return code: %d\n\n", rc);

    if (rc != 0) {
        fprintf(out, "Call failed (see gpu_amd.h for what code %d means).\n", rc);
    } else {
        fprintf(out, "AMD GPUs found: %d\n\n", data.device_count);
        for (int i = 0; i < data.device_count; i++) {
            const GPU_AMD_Device *d = &data.devices[i];
            fprintf(out, "Device %d:\n", i);
            fprintf(out, "  name:              %s\n", d->name);
            fprintf(out, "  pci_vendor_id:     0x%04x\n", d->pci_vendor_id);
            fprintf(out, "  pci_device_id:     0x%04x\n", d->pci_device_id);
            fprintf(out, "  vram_total_bytes:  %llu (%.2f GB)\n",
                    (unsigned long long)d->vram_total_bytes,
                    (double)d->vram_total_bytes / 1e9);
            fprintf(out, "  vram_used_bytes:   %llu\n", (unsigned long long)d->vram_used_bytes);
            fprintf(out, "  gtt_total_bytes:   %llu (%.2f GB)\n",
                    (unsigned long long)d->gtt_total_bytes, (double)d->gtt_total_bytes / 1e9);
            fprintf(out, "  engine_clock_mhz:  %d%s\n", d->engine_clock_mhz,
                    d->engine_clock_mhz < 0 ? " (not available)" : "");
            fprintf(out, "  memory_clock_mhz:  %d%s\n\n", d->memory_clock_mhz,
                    d->memory_clock_mhz < 0 ? " (not available)" : "");
        }
        free_amd_gpu_data(&data);
    }

    fclose(out);

    printf("Done. Wrote %s in this folder.\n", out_path);
    printf("Please send that file back so the results can be checked against your actual GPU.\n");
    return 0;
}
