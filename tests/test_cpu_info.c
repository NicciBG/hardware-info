#include "../include/cpu_info.h"

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
    CPU_DATA data;
    int rc = get_cpu_data(&data);
    check_int("get_cpu_data succeeds", rc, 0);
    if (rc != 0) return 1;

    printf("  cpu_name: %s\n", data.cpu_name);
    check_true("cpu_name is non-empty", data.cpu_name && strlen(data.cpu_name) > 0);

    check_int("logical_core_count", data.logical_core_count, 20);
    check_int("physical_core_count (6 HT P-cores + 8 E-cores)", data.physical_core_count, 14);

    int performance = 0, efficiency = 0, unknown = 0;
    for (int i = 0; i < data.physical_core_count; i++) {
        switch (data.cores[i].type) {
            case CORE_TYPE_PERFORMANCE: performance++; break;
            case CORE_TYPE_EFFICIENCY: efficiency++; break;
            case CORE_TYPE_UNKNOWN: unknown++; break;
        }
    }
    check_int("performance cores classified", performance, 6);
    check_int("efficiency cores classified", efficiency, 8);
    check_int("no cores left unclassified", unknown, 0);

    int logical_from_cores = 0;
    for (int i = 0; i < data.physical_core_count; i++) {
        logical_from_cores += data.cores[i].logical_count;
    }
    check_int("physical cores' logical_ids sum to logical_core_count",
              logical_from_cores, data.logical_core_count);

    check_int("cpu0 L1 data (P-core)", data.l1data[0], 48);
    check_int("cpu0 L1 instruction (P-core)", data.l1instruction[0], 32);
    check_int("cpu12 L1 data (E-core)", data.l1data[12], 32);
    check_int("cpu12 L1 instruction (E-core)", data.l1instruction[12], 64);

    check_int("8 distinct L2 groups (6 P-core pairs + 2 E-core clusters)",
              data.l2_group_count, 8);
    int p_pairs = 0, e_clusters = 0, logical_from_l2 = 0;
    for (int i = 0; i < data.l2_group_count; i++) {
        L2_Cache_Group *g = &data.l2_groups[i];
        logical_from_l2 += g->logical_count;
        if (g->size_kib == 2048 && g->logical_count == 2) p_pairs++;
        if (g->size_kib == 4096 && g->logical_count == 4) e_clusters++;
    }
    check_int("6 P-core L2 pairs (2048K, 2 logical each)", p_pairs, 6);
    check_int("2 E-core L2 clusters (4096K, 4 logical each)", e_clusters, 2);
    check_int("L2 groups' logical_ids sum to logical_core_count", logical_from_l2,
              data.logical_core_count);

    check_int("l3size (24MB shared)", data.l3size, 24576);

    printf("  algorithms: SSE4_2=%d AVX=%d AVX2=%d FMA=%d BMI1=%d BMI2=%d AVX512F=%d\n",
           data.algorithms.SSE4_2, data.algorithms.AVX, data.algorithms.AVX2,
           data.algorithms.FMA, data.algorithms.BMI1, data.algorithms.BMI2,
           data.algorithms.AVX512F);
    check_true("SSE4_2 detected", data.algorithms.SSE4_2);
    check_true("AVX detected", data.algorithms.AVX);
    check_true("AVX2 detected", data.algorithms.AVX2);
    check_true("FMA detected", data.algorithms.FMA);
    check_true("BMI1 detected", data.algorithms.BMI1);
    check_true("BMI2 detected", data.algorithms.BMI2);
    check_true("AVX512F correctly absent (fused off on this hybrid CPU)",
               !data.algorithms.AVX512F);

    free_cpu_data(&data);
    check_true("cpu_name null after free", data.cpu_name == nullptr);
    check_true("cores null after free", data.cores == nullptr);
    check_true("l2_groups null after free", data.l2_groups == nullptr);

    printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) failed.\n", failures);
    return failures == 0 ? 0 : 1;
}
