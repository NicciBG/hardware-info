#define _XOPEN_SOURCE 700

#include "gpu_qualcomm_platform.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* gpuclk / max_gpuclk report Hz (kgsl_pwrctrl.c: "%ld\n" / "%d\n" of a
   frequency in Hz), not MHz — convert here so this library's units match
   the others (cpu_info, gpu_amd, gpu_intel, gpu_nvidia all report MHz). */
static int read_hz_as_mhz(const char *path) {
    char buf[32];
    if (!read_sysfs_line(path, buf, sizeof buf)) return -1;
    long hz = strtol(buf, nullptr, 10);
    return hz > 0 ? (int)(hz / 1000000) : -1;
}

/* gpubusy is "%7d %7d\n" of (busy_old, total_old) cycle counts
   (kgsl_pwrctrl.c). Converts to a percentage; -1 if unavailable or the
   total was 0 (GPU idle/off resets both to 0, not a read failure). */
static int read_busy_percent(const char *path) {
    char buf[32];
    if (!read_sysfs_line(path, buf, sizeof buf)) return -1;
    long busy = 0, total = 0;
    if (sscanf(buf, "%ld %ld", &busy, &total) != 2 || total <= 0) return -1;
    return (int)((busy * 100) / total);
}

int platform_get_qualcomm_gpus(GPU_QUALCOMM_DATA *data) {
    DIR *dir = opendir("/sys/class/kgsl");
    if (!dir) return 0; /* no KGSL subsystem at all -> zero devices */

    size_t cap = 2;
    data->devices = malloc(cap * sizeof(GPU_QUALCOMM_Device));
    data->device_count = 0;
    if (!data->devices) {
        closedir(dir);
        return 203;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "kgsl-3d", 7) != 0) continue;

        char node_dir[300];
        snprintf(node_dir, sizeof node_dir, "/sys/class/kgsl/%s", entry->d_name);

        char sub[350];
        snprintf(sub, sizeof sub, "%s/gpu_model", node_dir);
        char model_buf[128] = {0};
        read_sysfs_line(sub, model_buf, sizeof model_buf);
        char *name = strdup(model_buf[0] ? model_buf : "Qualcomm Adreno GPU");
        if (!name) {
            closedir(dir);
            return 203;
        }

        snprintf(sub, sizeof sub, "%s/gpuclk", node_dir);
        int current_clock = read_hz_as_mhz(sub);
        snprintf(sub, sizeof sub, "%s/max_gpuclk", node_dir);
        int max_clock = read_hz_as_mhz(sub);
        snprintf(sub, sizeof sub, "%s/gpubusy", node_dir);
        int busy_percent = read_busy_percent(sub);

        if ((size_t)data->device_count >= cap) {
            cap *= 2;
            GPU_QUALCOMM_Device *grown =
                realloc(data->devices, cap * sizeof(GPU_QUALCOMM_Device));
            if (!grown) {
                free(name);
                closedir(dir);
                return 203;
            }
            data->devices = grown;
        }

        data->devices[data->device_count++] = (GPU_QUALCOMM_Device){
            .name = name,
            .current_clock_mhz = current_clock,
            .max_clock_mhz = max_clock,
            .busy_percent = busy_percent,
        };
    }

    closedir(dir);
    return 0;
}
