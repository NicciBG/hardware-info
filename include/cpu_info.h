#ifndef CPU_INFO_H
#define CPU_INFO_H

/* Public interface for the CPU layer of the hardware-probing library.
   Structure carried over from NicciBG/Computer_info (src/cpu_info.c) with
   two changes worked out before implementing:
     - l1size (ambiguous, and in the original Linux code actually ended up
       holding the instruction cache size due to processing order) split
       into l1data / l1instruction, both still per logical core.
     - L2 changed from a per-logical-core array to a group structure,
       since L2 is shared across a cluster of cores (a P-core's own two
       hyperthreads on one machine, 4 E-cores at a time on another — sizes
       differ by cluster too), not owned individually per core the way L1
       is.
   L3 stays a single shared value, as in the original — not something we
   found evidence needs generalizing here. */

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CORE_TYPE_PERFORMANCE,
    CORE_TYPE_EFFICIENCY,
    CORE_TYPE_UNKNOWN
} CoreType;

/* One entry per physical core: ID, type, and its logical-core indices. */
typedef struct {
    int id;
    CoreType type;
    int *logical_ids;
    int logical_count;
} PhysicalCoreInfo;

/* One entry per distinct L2 instance: its size and which logical cores
   share it. On a machine where every core has its own private L2, this
   is just one group per core; where L2 is shared by a cluster, each
   group's logical_ids lists every member of that cluster once. */
typedef struct {
    int size_kib;
    int *logical_ids;
    int logical_count;
} L2_Cache_Group;

/* Instruction-set flags, split by vendor relevance (from the original
   repo, char flags promoted to bool). */
typedef struct {
    /* common to both vendors */
    bool SSE;
    bool SSE2;
    bool SSE3;
    bool SSSE3;
    bool SSE4_1;
    bool SSE4_2;
    bool AVX;

    /* Intel-only */
    bool POPCNT;
    bool PCLMULQDQ;
    bool AES;
    bool FMA; /* FMA3 */
    bool F16C;
    bool XSAVE;
    bool OSXSAVE;
    bool RDRAND;
    bool RDSEED;
    bool ADX;
    bool MPX;
    bool PREFETCHWT1;

    /* common leaf 7 */
    bool AVX2;
    bool BMI1;
    bool BMI2;
    bool AVX512F;
    bool SHA;

    /* AMD-only */
    bool SSE4A;
    bool XOP;
    bool FMA4;
    bool THREEDNOW_PLUS;
} CPU_Algorithms;

typedef struct {
    char *cpu_name; /* brand string */
    int logical_core_count;
    int physical_core_count;
    PhysicalCoreInfo *cores; /* length = physical_core_count */

    int *l1data;        /* per logical core, KiB */
    int *l1instruction; /* per logical core, KiB */
    int *frequency;     /* per logical core, current MHz */

    L2_Cache_Group *l2_groups;
    int l2_group_count;

    int l3size; /* KiB, shared */

    CPU_Algorithms algorithms;
} CPU_DATA;

/* Allocate and zero a CPU_DATA before calling. Free with free_cpu_data(),
   not by hand — it's exported so a caller across a shared-library
   boundary frees with the same allocator that made the allocations,
   rather than risking a heap mismatch (this matters in particular on
   Windows, where a DLL and its caller are not guaranteed to share a CRT
   heap). */
DLL_EXPORT int get_cpu_data(CPU_DATA *data);
DLL_EXPORT void free_cpu_data(CPU_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* CPU_INFO_H */

/*
0     Success
201   Null pointer passed to get_cpu_data
202   Failed to open a required system info source
203   Memory allocation failure
204-207  Platform-specific API failures
208   Allocation failure for an internal array
*/
