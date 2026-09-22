#include "../include/gpu_qualcomm.h"

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
    /* No Qualcomm/KGSL hardware exists anywhere this has been developed
       or run — this only verifies the graceful "not present" path, same
       as every other run of this test until real hardware is checked. */
    GPU_QUALCOMM_DATA data;
    int rc = get_qualcomm_gpu_data(&data);
    check_int("get_qualcomm_gpu_data succeeds", rc, 0);
    check_int("no KGSL on this machine -> zero devices", data.device_count, 0);

    free_qualcomm_gpu_data(&data);
    check_true("devices null after free", data.devices == nullptr);

    printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) failed.\n", failures);
    return failures == 0 ? 0 : 1;
}
