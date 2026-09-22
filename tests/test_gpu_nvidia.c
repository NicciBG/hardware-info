#include "../include/gpu_nvidia.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check_true(const char *what, bool cond) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    } else {
        printf("ok   %s\n", what);
    }
}

static void check_int(const char *what, int got, int want) {
    check_true(what, got == want);
    if (got != want) printf("     got %d, want %d\n", got, want);
}

int main(void) {
    GPU_NVIDIA_DATA data;
    int rc = get_nvidia_gpu_data(&data);
    check_int("get_nvidia_gpu_data succeeds", rc, 0);
    if (rc != 0) return 1;

    printf("  nvidia_smi_available=%d\n", data.nvidia_smi_available);
    check_true("at least one NVIDIA GPU found (this machine's RTX 3080)", data.device_count >= 1);

    bool found_3080 = false;
    for (int i = 0; i < data.device_count; i++) {
        GPU_NVIDIA_Device *d = &data.devices[i];
        printf("  [%d] name=\"%s\" uuid=%s bus=%s vendor=0x%04x device=0x%04x\n", i, d->name,
               d->uuid, d->pci_bus_location, d->pci_vendor_id, d->pci_device_id);
        printf("       vram_total=%llu vram_used=%llu core_mhz=%d mem_mhz=%d\n",
               (unsigned long long)d->vram_total_bytes, (unsigned long long)d->vram_used_bytes,
               d->core_clock_mhz, d->memory_clock_mhz);

        check_int("vendor is NVIDIA", (int)d->pci_vendor_id, 0x10de);
        if (strstr(d->name, "RTX 3080")) {
            found_3080 = true;
            check_int("device id matches lspci (0x2216)", (int)d->pci_device_id, 0x2216);
            check_true("uuid starts with GPU-", strncmp(d->uuid, "GPU-", 4) == 0);
            check_true("bus location is 0000:01:00.0",
                       strcmp(d->pci_bus_location, "0000:01:00.0") == 0);
            if (data.nvidia_smi_available) {
                check_true("vram_total ~10GB (RTX 3080)",
                           d->vram_total_bytes > 9000000000ULL &&
                               d->vram_total_bytes < 11000000000ULL);
                check_true("core_clock_mhz is plausible (>0)", d->core_clock_mhz > 0);
                check_true("memory_clock_mhz is plausible (>0)", d->memory_clock_mhz > 0);
            }
        }
    }
    check_true("RTX 3080 specifically found", found_3080);
    check_true("nvidia-smi was available on this machine", data.nvidia_smi_available);

    free_nvidia_gpu_data(&data);
    check_true("devices null after free", data.devices == nullptr);
    check_int("device_count 0 after free", data.device_count, 0);

    printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) failed.\n", failures);
    return failures == 0 ? 0 : 1;
}
