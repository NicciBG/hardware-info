#define _XOPEN_SOURCE 700 /* sysconf(_SC_NPROCESSORS_ONLN) */

#include "cpu_info_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool read_sysfs_line(const char *path, char *buf, size_t buf_size) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    bool ok = fgets(buf, (int)buf_size, f) != nullptr;
    fclose(f);
    if (ok) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
    }
    return ok;
}

static int read_sysfs_int(const char *path, int default_value) {
    char buf[64];
    if (!read_sysfs_line(path, buf, sizeof buf)) return default_value;
    return atoi(buf);
}

static int parse_cache_size_kib(const char *raw) {
    int value = atoi(raw);
    if (strchr(raw, 'M') || strchr(raw, 'm')) value *= 1024;
    return value;
}

/* Parses Linux's cpu-list format ("0-11", "12-15,16-19", ...) into a flat
   list of individual ids. Shared by shared_cpu_list parsing and the
   cpu_core/cpu_atom membership files. */
static int parse_cpu_range_list(const char *str, int *out_ids, int max_out) {
    int count = 0;
    const char *p = str;
    while (*p && count < max_out) {
        char *end;
        long start = strtol(p, &end, 10);
        if (end == p) break;
        long finish = start;
        p = end;
        if (*p == '-') {
            p++;
            finish = strtol(p, &end, 10);
            if (end == p) break;
            p = end;
        }
        for (long v = start; v <= finish && count < max_out; v++) {
            out_ids[count++] = (int)v;
        }
        if (*p == ',') p++;
        else break;
    }
    return count;
}

int platform_get_logical_core_count(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 0;
}

int platform_populate_topology(CPU_DATA *data) {
    int L = data->logical_core_count;

    bool *p_core_mask = calloc((size_t)L, sizeof(bool));
    bool *e_core_mask = calloc((size_t)L, sizeof(bool));
    if (!p_core_mask || !e_core_mask) {
        free(p_core_mask);
        free(e_core_mask);
        return 203;
    }

    char listbuf[256];
    int ids[256];
    if (read_sysfs_line("/sys/devices/cpu_core/cpus", listbuf, sizeof listbuf)) {
        int n = parse_cpu_range_list(listbuf, ids, 256);
        for (int i = 0; i < n; i++) {
            if (ids[i] >= 0 && ids[i] < L) p_core_mask[ids[i]] = true;
        }
    }
    if (read_sysfs_line("/sys/devices/cpu_atom/cpus", listbuf, sizeof listbuf)) {
        int n = parse_cpu_range_list(listbuf, ids, 256);
        for (int i = 0; i < n; i++) {
            if (ids[i] >= 0 && ids[i] < L) e_core_mask[ids[i]] = true;
        }
    }

    int *keys = calloc((size_t)L, sizeof(int));
    PhysicalCoreInfo *temp = calloc((size_t)L, sizeof(PhysicalCoreInfo));
    if (!keys || !temp) {
        free(p_core_mask);
        free(e_core_mask);
        free(keys);
        free(temp);
        return 203;
    }

    int unique = 0;
    for (int cpu = 0; cpu < L; cpu++) {
        char path[128];
        snprintf(path, sizeof path,
                 "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpu);
        int phy = read_sysfs_int(path, 0);
        snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/topology/core_id", cpu);
        int core = read_sysfs_int(path, 0);

        int key = (phy << 16) | (core & 0xFFFF);
        int idx = 0;
        while (idx < unique && keys[idx] != key) idx++;
        if (idx == unique) {
            keys[unique] = key;
            temp[unique].id = key;
            temp[unique].type = p_core_mask[cpu]   ? CORE_TYPE_PERFORMANCE
                                 : e_core_mask[cpu] ? CORE_TYPE_EFFICIENCY
                                                     : CORE_TYPE_UNKNOWN;
            temp[unique].logical_count = 0;
            unique++;
        }
        temp[idx].logical_count++;
    }

    data->physical_core_count = unique;
    data->cores = calloc((size_t)unique, sizeof(PhysicalCoreInfo));
    if (!data->cores) {
        free(p_core_mask);
        free(e_core_mask);
        free(keys);
        free(temp);
        return 203;
    }
    for (int i = 0; i < unique; i++) {
        data->cores[i] = temp[i];
        data->cores[i].logical_ids = malloc((size_t)temp[i].logical_count * sizeof(int));
        data->cores[i].logical_count = 0;
        if (!data->cores[i].logical_ids) {
            free(p_core_mask);
            free(e_core_mask);
            free(keys);
            free(temp);
            return 203;
        }
    }

    for (int cpu = 0; cpu < L; cpu++) {
        char path[128];
        snprintf(path, sizeof path,
                 "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpu);
        int phy = read_sysfs_int(path, 0);
        snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/topology/core_id", cpu);
        int core = read_sysfs_int(path, 0);
        int key = (phy << 16) | (core & 0xFFFF);
        int idx = 0;
        while (keys[idx] != key) idx++;
        data->cores[idx].logical_ids[data->cores[idx].logical_count++] = cpu;
    }

    free(p_core_mask);
    free(e_core_mask);
    free(keys);
    free(temp);
    return 0;
}

int platform_populate_caches_and_freq(CPU_DATA *data) {
    int L = data->logical_core_count;

    bool *covered = calloc((size_t)L, sizeof(bool));
    if (!covered) return 203;

    size_t group_cap = 4;
    data->l2_groups = malloc(group_cap * sizeof(L2_Cache_Group));
    data->l2_group_count = 0;
    if (!data->l2_groups) {
        free(covered);
        return 203;
    }

    for (int cpu = 0; cpu < L; cpu++) {
        char path[160];

        snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
                 cpu);
        data->frequency[cpu] = read_sysfs_int(path, 0) / 1000; /* kHz -> MHz */

        for (int idx = 0;; idx++) {
            char dir[128];
            snprintf(dir, sizeof dir, "/sys/devices/system/cpu/cpu%d/cache/index%d", cpu, idx);
            struct stat st;
            if (stat(dir, &st) != 0) break;

            snprintf(path, sizeof path, "%s/level", dir);
            int level = read_sysfs_int(path, 0);
            snprintf(path, sizeof path, "%s/type", dir);
            char type[32] = {0};
            read_sysfs_line(path, type, sizeof type);
            snprintf(path, sizeof path, "%s/size", dir);
            char sizebuf[32] = {0};
            read_sysfs_line(path, sizebuf, sizeof sizebuf);
            int size_kib = parse_cache_size_kib(sizebuf);

            if (level == 1 && strcmp(type, "Data") == 0) {
                data->l1data[cpu] = size_kib;
            } else if (level == 1 && strcmp(type, "Instruction") == 0) {
                data->l1instruction[cpu] = size_kib;
            } else if (level == 2 && !covered[cpu]) {
                snprintf(path, sizeof path, "%s/shared_cpu_list", dir);
                char listbuf[128] = {0};
                read_sysfs_line(path, listbuf, sizeof listbuf);

                int ids[256];
                int n = parse_cpu_range_list(listbuf, ids, 256);
                if (n == 0) {
                    ids[0] = cpu;
                    n = 1;
                }

                if ((size_t)data->l2_group_count >= group_cap) {
                    group_cap *= 2;
                    L2_Cache_Group *grown =
                        realloc(data->l2_groups, group_cap * sizeof(L2_Cache_Group));
                    if (!grown) {
                        free(covered);
                        return 203;
                    }
                    data->l2_groups = grown;
                }

                int *members = malloc((size_t)n * sizeof(int));
                if (!members) {
                    free(covered);
                    return 203;
                }
                memcpy(members, ids, (size_t)n * sizeof(int));

                data->l2_groups[data->l2_group_count++] = (L2_Cache_Group){
                    .size_kib = size_kib,
                    .logical_ids = members,
                    .logical_count = n,
                };

                for (int i = 0; i < n; i++) {
                    if (ids[i] >= 0 && ids[i] < L) covered[ids[i]] = true;
                }
            } else if (level == 3) {
                data->l3size = size_kib;
            }
        }
    }

    free(covered);
    return 0;
}
