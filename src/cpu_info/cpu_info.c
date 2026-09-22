/* Common logic: CPUID-based brand/vendor/instruction-set detection, and
   the get_cpu_data() orchestration that hands off to whichever platform
   file (libcpu_info_linux.c / cpu_info_windows.c / libcpu_info_macos.c)
   is linked in for topology and cache/frequency enumeration. Nothing in
   this file is OS-specific. */

#include "cpu_info.h"
#include "cpu_info_platform.h"

#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <immintrin.h>
#endif

static inline void cpu_cpuid(unsigned int leaf, unsigned int subleaf, int regs[4]) {
#if defined(_MSC_VER)
    __cpuidex(regs, (int)leaf, (int)subleaf);
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("cpuid"
                      : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
                      : "a"(leaf), "c"(subleaf));
#else
    regs[0] = regs[1] = regs[2] = regs[3] = 0;
#endif
}

static int get_cpu_brand(char **out_name) {
    char *brand = malloc(49);
    if (!brand) return 203;

    int regs[4];
    char *p = brand;
    for (unsigned int leaf = 0; leaf < 3; ++leaf) {
        cpu_cpuid(0x80000002 + leaf, 0, regs);
        memcpy(p, regs, 16);
        p += 16;
    }
    brand[48] = '\0';
    *out_name = brand;
    return 0;
}

static void get_cpu_vendor(char vendor[13]) {
    int regs[4];
    cpu_cpuid(0, 0, regs);
    memcpy(vendor + 0, &regs[1], 4);
    memcpy(vendor + 4, &regs[3], 4);
    memcpy(vendor + 8, &regs[2], 4);
    vendor[12] = '\0';
}

static void get_supported_algorithms(CPU_Algorithms *alg) {
    int regs[4];
    char vendor[13];
    *alg = (CPU_Algorithms){0};

    get_cpu_vendor(vendor);
    bool is_intel = strcmp(vendor, "GenuineIntel") == 0;
    bool is_amd = strcmp(vendor, "AuthenticAMD") == 0;

    cpu_cpuid(1, 0, regs);
    alg->SSE = (regs[3] & (1 << 25)) != 0;
    alg->SSE2 = (regs[3] & (1 << 26)) != 0;
    alg->SSE3 = (regs[2] & (1 << 0)) != 0;
    alg->SSSE3 = (regs[2] & (1 << 9)) != 0;
    alg->SSE4_1 = (regs[2] & (1 << 19)) != 0;
    alg->SSE4_2 = (regs[2] & (1 << 20)) != 0;
    alg->AVX = (regs[2] & (1 << 28)) != 0;

    if (is_intel) {
        alg->POPCNT = (regs[2] & (1 << 23)) != 0;
        alg->PCLMULQDQ = (regs[2] & (1 << 1)) != 0;
        alg->AES = (regs[2] & (1 << 25)) != 0;
        alg->FMA = (regs[2] & (1 << 12)) != 0;
        alg->F16C = (regs[2] & (1 << 29)) != 0;
        alg->XSAVE = (regs[2] & (1 << 26)) != 0;
        alg->OSXSAVE = (regs[2] & (1 << 27)) != 0;
        alg->RDRAND = (regs[2] & (1 << 30)) != 0;
    }

    cpu_cpuid(7, 0, regs);
    alg->AVX2 = (regs[1] & (1 << 5)) != 0;
    alg->BMI1 = (regs[1] & (1 << 3)) != 0;
    alg->BMI2 = (regs[1] & (1 << 8)) != 0;
    alg->AVX512F = (regs[1] & (1 << 16)) != 0;
    alg->SHA = (regs[2] & (1 << 29)) != 0;

    if (is_intel) {
        alg->RDSEED = (regs[1] & (1 << 18)) != 0;
        alg->ADX = (regs[1] & (1 << 19)) != 0;
        alg->MPX = (regs[1] & (1 << 14)) != 0;
        alg->PREFETCHWT1 = (regs[2] & (1 << 0)) != 0;
    }

    if (is_amd) {
        cpu_cpuid(0x80000001, 0, regs);
        alg->SSE4A = (regs[2] & (1 << 6)) != 0;
        alg->XOP = (regs[2] & (1 << 11)) != 0;
        alg->FMA4 = (regs[2] & (1 << 16)) != 0;
        alg->THREEDNOW_PLUS = (((unsigned)regs[3]) & (1U << 31)) != 0;
    }
}

int get_cpu_data(CPU_DATA *data) {
    if (!data) return 201;
    *data = (CPU_DATA){0};

    int rc = get_cpu_brand(&data->cpu_name);
    if (rc != 0) return rc;

    get_supported_algorithms(&data->algorithms);

    data->logical_core_count = platform_get_logical_core_count();
    if (data->logical_core_count <= 0) return 202;

    data->l1data = calloc((size_t)data->logical_core_count, sizeof(int));
    data->l1instruction = calloc((size_t)data->logical_core_count, sizeof(int));
    data->frequency = calloc((size_t)data->logical_core_count, sizeof(int));
    if (!data->l1data || !data->l1instruction || !data->frequency) {
        free_cpu_data(data);
        return 203;
    }

    rc = platform_populate_caches_and_freq(data);
    if (rc != 0) {
        free_cpu_data(data);
        return rc;
    }

    rc = platform_populate_topology(data);
    if (rc != 0) {
        free_cpu_data(data);
        return rc;
    }

    return 0;
}

void free_cpu_data(CPU_DATA *data) {
    if (!data) return;

    free(data->cpu_name);
    free(data->l1data);
    free(data->l1instruction);
    free(data->frequency);

    for (int i = 0; i < data->physical_core_count; i++) {
        free(data->cores[i].logical_ids);
    }
    free(data->cores);

    for (int i = 0; i < data->l2_group_count; i++) {
        free(data->l2_groups[i].logical_ids);
    }
    free(data->l2_groups);

    *data = (CPU_DATA){0};
}
