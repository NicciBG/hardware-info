#define _XOPEN_SOURCE 700

#include "gpu_amd_platform.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AMD_PCI_VENDOR_ID 0x1002u

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

static uint64_t read_sysfs_u64(const char *path, uint64_t default_value) {
    char buf[64];
    if (!read_sysfs_line(path, buf, sizeof buf)) return default_value;
    return strtoull(buf, nullptr, 10);
}

static unsigned int read_sysfs_hex(const char *path, unsigned int default_value) {
    char buf[32];
    if (!read_sysfs_line(path, buf, sizeof buf)) return default_value;
    return (unsigned int)strtoul(buf, nullptr, 16);
}

/* pp_dpm_sclk / pp_dpm_mclk list every supported clock level, one per
   line, like:
     0: 300Mhz
     1: 600Mhz
     2: 900Mhz *
   The line ending in '*' (after trimming) is the currently active level.
   Returns -1 if the file is absent or no line is marked active, rather
   than guessing. */
static int read_active_dpm_clock_mhz(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[128];
    int result = -1;
    while (fgets(line, sizeof line, f)) {
        size_t len = strlen(line);
        while (len > 0 && isspace((unsigned char)line[len - 1])) len--;
        line[len] = '\0';
        if (len == 0 || line[len - 1] != '*') continue;

        const char *colon = strchr(line, ':');
        if (!colon) continue;
        result = atoi(colon + 1);
        break;
    }
    fclose(f);
    return result;
}

static char *read_product_name_or_fallback(const char *device_dir, unsigned int device_id) {
    char path[400];
    snprintf(path, sizeof path, "%s/product_name", device_dir);
    char buf[128];
    if (read_sysfs_line(path, buf, sizeof buf) && buf[0] != '\0') {
        size_t len = strlen(buf);
        while (len > 0 && isspace((unsigned char)buf[len - 1])) buf[--len] = '\0';
        if (len > 0) return strdup(buf);
    }
    char fallback[64];
    snprintf(fallback, sizeof fallback, "AMD GPU (device 0x%04x)", device_id);
    return strdup(fallback);
}

int platform_get_amd_gpus(GPU_AMD_DATA *data) {
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) return 0; /* no DRM subsystem at all -> zero devices, not an error */

    size_t cap = 2;
    data->devices = malloc(cap * sizeof(GPU_AMD_Device));
    data->device_count = 0;
    if (!data->devices) {
        closedir(dir);
        return 203;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        /* Only bare "cardN" entries are GPUs; "cardN-DP-1" etc. are
           connector sub-entries, not separate devices. */
        if (strncmp(entry->d_name, "card", 4) != 0) continue;
        const char *rest = entry->d_name + 4;
        if (*rest == '\0' || !isdigit((unsigned char)*rest)) continue;
        bool all_digits = true;
        for (const char *p = rest; *p; p++) {
            if (!isdigit((unsigned char)*p)) {
                all_digits = false;
                break;
            }
        }
        if (!all_digits) continue;

        char device_dir[300];
        snprintf(device_dir, sizeof device_dir, "/sys/class/drm/%s/device", entry->d_name);

        char sub[350];
        snprintf(sub, sizeof sub, "%s/vendor", device_dir);
        unsigned int vendor = read_sysfs_hex(sub, 0);
        if (vendor != AMD_PCI_VENDOR_ID) continue;

        snprintf(sub, sizeof sub, "%s/device", device_dir);
        unsigned int device_id = read_sysfs_hex(sub, 0);

        char *name = read_product_name_or_fallback(device_dir, device_id);
        if (!name) {
            closedir(dir);
            return 203;
        }

        snprintf(sub, sizeof sub, "%s/mem_info_vram_total", device_dir);
        uint64_t vram_total = read_sysfs_u64(sub, 0);
        snprintf(sub, sizeof sub, "%s/mem_info_vram_used", device_dir);
        uint64_t vram_used = read_sysfs_u64(sub, 0);
        snprintf(sub, sizeof sub, "%s/mem_info_gtt_total", device_dir);
        uint64_t gtt_total = read_sysfs_u64(sub, 0);

        snprintf(sub, sizeof sub, "%s/pp_dpm_sclk", device_dir);
        int engine_clock = read_active_dpm_clock_mhz(sub);
        snprintf(sub, sizeof sub, "%s/pp_dpm_mclk", device_dir);
        int memory_clock = read_active_dpm_clock_mhz(sub);

        if ((size_t)data->device_count >= cap) {
            cap *= 2;
            GPU_AMD_Device *grown = realloc(data->devices, cap * sizeof(GPU_AMD_Device));
            if (!grown) {
                free(name);
                closedir(dir);
                return 203;
            }
            data->devices = grown;
        }

        data->devices[data->device_count++] = (GPU_AMD_Device){
            .name = name,
            .pci_vendor_id = vendor,
            .pci_device_id = device_id,
            .vram_total_bytes = vram_total,
            .vram_used_bytes = vram_used,
            .gtt_total_bytes = gtt_total,
            .engine_clock_mhz = engine_clock,
            .memory_clock_mhz = memory_clock,
        };
    }

    closedir(dir);
    return 0;
}
