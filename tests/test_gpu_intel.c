#include "../include/gpu_intel.h"

#include <stdio.h>

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
    GPU_INTEL_DATA data;
    int rc = get_intel_gpu_data(&data);
    check_int("get_intel_gpu_data succeeds", rc, 0);
    if (rc != 0) return 1;

    check_true("at least one Intel GPU found (this machine's UHD 770)", data.device_count >= 1);

    bool found_uhd770 = false;
    for (int i = 0; i < data.device_count; i++) {
        GPU_INTEL_Device *d = &data.devices[i];
        printf("  [%d] name=%s vendor=0x%04x device=0x%04x max=%d min=%d cur=%d\n", i, d->name,
               d->pci_vendor_id, d->pci_device_id, d->max_freq_mhz, d->min_freq_mhz,
               d->current_freq_mhz);
        check_int("vendor is Intel", (int)d->pci_vendor_id, 0x8086);
        if (d->pci_device_id == 0xa780) {
            found_uhd770 = true;
            check_int("UHD 770 max_freq_mhz", d->max_freq_mhz, 1550);
            check_int("UHD 770 min_freq_mhz", d->min_freq_mhz, 300);
            check_true("UHD 770 current_freq_mhz is a plausible reading (0..max)",
                       d->current_freq_mhz >= 0 && d->current_freq_mhz <= d->max_freq_mhz);
        }
    }
    check_true("UHD 770 (device 0xa780) specifically found", found_uhd770);

    free_intel_gpu_data(&data);
    check_true("devices null after free", data.devices == nullptr);
    check_int("device_count 0 after free", data.device_count, 0);

    printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) failed.\n", failures);
    return failures == 0 ? 0 : 1;
}
