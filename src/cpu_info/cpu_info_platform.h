#ifndef CPU_INFO_PLATFORM_H
#define CPU_INFO_PLATFORM_H

/* Internal seam between the common logic (cpu_info.c) and each platform's
   implementation (libcpu_info_linux.c / cpu_info_windows.c /
   libcpu_info_macos.c). Not part of the public API — not installed,
   not exported. */

#include "cpu_info.h"

int platform_get_logical_core_count(void);

/* Fills physical_core_count and cores[] (including CoreType where the
   platform can tell performance and efficiency cores apart). */
int platform_populate_topology(CPU_DATA *data);

/* Fills l1data/l1instruction/frequency (already allocated by the caller,
   length logical_core_count), l2_groups/l2_group_count, and l3size. */
int platform_populate_caches_and_freq(CPU_DATA *data);

#endif /* CPU_INFO_PLATFORM_H */
