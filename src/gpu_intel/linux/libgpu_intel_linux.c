#define _XOPEN_SOURCE 700

#include "gpu_intel_platform.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INTEL_PCI_VENDOR_ID 0x8086u

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

static unsigned int read_sysfs_hex(const char *path, unsigned int default_value) {
    char buf[32];
    if (!read_sysfs_line(path, buf, sizeof buf)) return default_value;
    return (unsigned int)strtoul(buf, nullptr, 16);
}

/* Newer kernels expose per-GT frequency files under gt/gt0/rps_*; older
   ones only have the flat gt_*_freq_mhz files directly under the card
   directory. Try the modern path first, fall back to the legacy one. */
static int read_freq_with_fallback(const char *card_dir, const char *new_name,
                                    const char *legacy_name) {
    char path[350];
    char buf[32];

    snprintf(path, sizeof path, "%s/gt/gt0/%s", card_dir, new_name);
    if (read_sysfs_line(path, buf, sizeof buf)) return atoi(buf);

    snprintf(path, sizeof path, "%s/%s", card_dir, legacy_name);
    if (read_sysfs_line(path, buf, sizeof buf)) return atoi(buf);

    return -1;
}

int platform_get_intel_gpus(GPU_INTEL_DATA *data) {
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) return 0;

    size_t cap = 2;
    data->devices = malloc(cap * sizeof(GPU_INTEL_Device));
    data->device_count = 0;
    if (!data->devices) {
        closedir(dir);
        return 203;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "card", 4) != 0) continue;
        const char *rest = entry->d_name + 4;
        if (*rest == '\0') continue;
        bool all_digits = true;
        for (const char *p = rest; *p; p++) {
            if (!isdigit((unsigned char)*p)) {
                all_digits = false;
                break;
            }
        }
        if (!all_digits) continue;

        char card_dir[300];
        snprintf(card_dir, sizeof card_dir, "/sys/class/drm/%s", entry->d_name);

        char sub[350];
        snprintf(sub, sizeof sub, "%s/device/vendor", card_dir);
        unsigned int vendor = read_sysfs_hex(sub, 0);
        if (vendor != INTEL_PCI_VENDOR_ID) continue;

        snprintf(sub, sizeof sub, "%s/device/device", card_dir);
        unsigned int device_id = read_sysfs_hex(sub, 0);

        char fallback_name[64];
        snprintf(fallback_name, sizeof fallback_name, "Intel GPU (device 0x%04x)", device_id);
        char *name = strdup(fallback_name);
        if (!name) {
            closedir(dir);
            return 203;
        }

        int max_freq = read_freq_with_fallback(card_dir, "rps_RP0_freq_mhz", "gt_RP0_freq_mhz");
        int min_freq = read_freq_with_fallback(card_dir, "rps_RPn_freq_mhz", "gt_RPn_freq_mhz");
        int cur_freq = read_freq_with_fallback(card_dir, "rps_cur_freq_mhz", "gt_cur_freq_mhz");

        if ((size_t)data->device_count >= cap) {
            cap *= 2;
            GPU_INTEL_Device *grown = realloc(data->devices, cap * sizeof(GPU_INTEL_Device));
            if (!grown) {
                free(name);
                closedir(dir);
                return 203;
            }
            data->devices = grown;
        }

        data->devices[data->device_count++] = (GPU_INTEL_Device){
            .name = name,
            .pci_vendor_id = vendor,
            .pci_device_id = device_id,
            .max_freq_mhz = max_freq,
            .min_freq_mhz = min_freq,
            .current_freq_mhz = cur_freq,
        };
    }

    closedir(dir);
    return 0;
}
