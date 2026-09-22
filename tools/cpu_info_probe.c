/* Standalone verification tool for libcpu_info_*. Same pattern as
   tools/gpu_amd_probe.c — writes everything detected to a plain text
   file, meant to be run on hardware the maintainer doesn't have (e.g. a
   different core-count/topology, or a CPU vendor other than the one
   this was developed against). */

#include "cpu_info.h"

#include <stdio.h>
#include <time.h>

static const char *core_type_name(CoreType t) {
    switch (t) {
        case CORE_TYPE_PERFORMANCE: return "performance";
        case CORE_TYPE_EFFICIENCY: return "efficiency";
        default: return "unknown";
    }
}

int main(void) {
    const char *out_path = "cpu_info_probe_result.txt";
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Could not create %s in the current folder.\n", out_path);
        return 1;
    }

    time_t now = time(nullptr);
    fprintf(out, "CPU probe result\n");
    fprintf(out, "generated: %s", ctime(&now));

    CPU_DATA data;
    int rc = get_cpu_data(&data);
    fprintf(out, "get_cpu_data return code: %d\n\n", rc);

    if (rc != 0) {
        fprintf(out, "Call failed (see cpu_info.h for what code %d means).\n", rc);
        fclose(out);
        printf("Done. Wrote %s in this folder (call failed, see the file).\n", out_path);
        return 0;
    }

    fprintf(out, "cpu_name: %s\n", data.cpu_name);
    fprintf(out, "logical_core_count: %d\n", data.logical_core_count);
    fprintf(out, "physical_core_count: %d\n\n", data.physical_core_count);

    fprintf(out, "Physical cores:\n");
    for (int i = 0; i < data.physical_core_count; i++) {
        PhysicalCoreInfo *c = &data.cores[i];
        fprintf(out, "  core %d: type=%s logical_ids=[", i, core_type_name(c->type));
        for (int j = 0; j < c->logical_count; j++) {
            fprintf(out, "%d%s", c->logical_ids[j], j + 1 < c->logical_count ? "," : "");
        }
        fprintf(out, "]\n");
    }

    fprintf(out, "\nPer logical core:\n");
    for (int i = 0; i < data.logical_core_count; i++) {
        fprintf(out, "  cpu%d: l1data=%dKiB l1instruction=%dKiB frequency=%dMHz\n", i,
                data.l1data[i], data.l1instruction[i], data.frequency[i]);
    }

    fprintf(out, "\nL2 cache groups: %d\n", data.l2_group_count);
    for (int i = 0; i < data.l2_group_count; i++) {
        L2_Cache_Group *g = &data.l2_groups[i];
        fprintf(out, "  group %d: %dKiB, shared by logical_ids=[", i, g->size_kib);
        for (int j = 0; j < g->logical_count; j++) {
            fprintf(out, "%d%s", g->logical_ids[j], j + 1 < g->logical_count ? "," : "");
        }
        fprintf(out, "]\n");
    }

    fprintf(out, "\nl3size: %dKiB (shared)\n\n", data.l3size);

    fprintf(out, "Instruction set flags:\n");
#define PRINT_FLAG(f) fprintf(out, "  %-14s %s\n", #f, data.algorithms.f ? "yes" : "no")
    PRINT_FLAG(SSE);
    PRINT_FLAG(SSE2);
    PRINT_FLAG(SSE3);
    PRINT_FLAG(SSSE3);
    PRINT_FLAG(SSE4_1);
    PRINT_FLAG(SSE4_2);
    PRINT_FLAG(AVX);
    PRINT_FLAG(AVX2);
    PRINT_FLAG(FMA);
    PRINT_FLAG(BMI1);
    PRINT_FLAG(BMI2);
    PRINT_FLAG(AVX512F);
    PRINT_FLAG(SHA);
    PRINT_FLAG(POPCNT);
    PRINT_FLAG(PCLMULQDQ);
    PRINT_FLAG(AES);
    PRINT_FLAG(F16C);
    PRINT_FLAG(RDRAND);
    PRINT_FLAG(RDSEED);
    PRINT_FLAG(SSE4A);
    PRINT_FLAG(XOP);
    PRINT_FLAG(FMA4);
    PRINT_FLAG(THREEDNOW_PLUS);
#undef PRINT_FLAG

    free_cpu_data(&data);
    fclose(out);

    printf("Done. Wrote %s in this folder.\n", out_path);
    printf("Please send that file back so the results can be checked against your actual CPU.\n");
    return 0;
}
