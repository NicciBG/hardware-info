#define _XOPEN_SOURCE 700

#include "gpu_nvidia_platform.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static unsigned int read_sysfs_hex(const char *path, unsigned int default_value) {
    FILE *f = fopen(path, "r");
    if (!f) return default_value;
    char buf[32];
    bool ok = fgets(buf, sizeof buf, f) != nullptr;
    fclose(f);
    return ok ? (unsigned int)strtoul(buf, nullptr, 16) : default_value;
}

static char *read_whole_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return nullptr;
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        fclose(f);
        return nullptr;
    }
    for (;;) {
        if (len + 1024 > cap) {
            cap *= 2;
            char *grown = realloc(buf, cap);
            if (!grown) {
                free(buf);
                fclose(f);
                return nullptr;
            }
            buf = grown;
        }
        size_t n = fread(buf + len, 1, cap - len - 1, f);
        len += n;
        if (n == 0) break;
    }
    fclose(f);
    buf[len] = '\0';
    return buf;
}

static bool extract_field(const char *content, const char *key, char *out, size_t out_size) {
    size_t key_len = strlen(key);
    for (const char *line = content; line; line = strchr(line, '\n') ? strchr(line, '\n') + 1 : nullptr) {
        if (strncmp(line, key, key_len) == 0 && line[key_len] == ':') {
            const char *value = line + key_len + 1;
            while (*value == ' ' || *value == '\t') value++;
            const char *end = strchr(value, '\n');
            size_t len = end ? (size_t)(end - value) : strlen(value);
            if (len >= out_size) len = out_size - 1;
            memcpy(out, value, len);
            out[len] = '\0';
            return true;
        }
    }
    return false;
}

/* ---------------- identification, via /proc/driver/nvidia ---------------- */

int platform_get_nvidia_gpus(GPU_NVIDIA_DATA *data) {
    DIR *dir = opendir("/proc/driver/nvidia/gpus");
    if (!dir) return 0; /* proprietary driver not loaded -> zero devices */

    size_t cap = 2;
    data->devices = malloc(cap * sizeof(GPU_NVIDIA_Device));
    data->device_count = 0;
    if (!data->devices) {
        closedir(dir);
        return 203;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;

        char info_path[300];
        snprintf(info_path, sizeof info_path, "/proc/driver/nvidia/gpus/%s/information",
                 entry->d_name);
        char *content = read_whole_file(info_path);
        if (!content) continue;

        char model[128] = {0}, uuid[80] = {0};
        extract_field(content, "Model", model, sizeof model);
        extract_field(content, "GPU UUID", uuid, sizeof uuid);
        free(content);

        char sysfs_path[512];
        snprintf(sysfs_path, sizeof sysfs_path, "/sys/bus/pci/devices/%s/vendor", entry->d_name);
        unsigned int vendor = read_sysfs_hex(sysfs_path, 0);
        snprintf(sysfs_path, sizeof sysfs_path, "/sys/bus/pci/devices/%s/device", entry->d_name);
        unsigned int device_id = read_sysfs_hex(sysfs_path, 0);

        char *name = strdup(model[0] ? model : "NVIDIA GPU");
        char *uuid_dup = strdup(uuid);
        char *bus_dup = strdup(entry->d_name);
        if (!name || !uuid_dup || !bus_dup) {
            free(name);
            free(uuid_dup);
            free(bus_dup);
            closedir(dir);
            return 203;
        }

        if ((size_t)data->device_count >= cap) {
            cap *= 2;
            GPU_NVIDIA_Device *grown = realloc(data->devices, cap * sizeof(GPU_NVIDIA_Device));
            if (!grown) {
                free(name);
                free(uuid_dup);
                free(bus_dup);
                closedir(dir);
                return 203;
            }
            data->devices = grown;
        }

        data->devices[data->device_count++] = (GPU_NVIDIA_Device){
            .name = name,
            .uuid = uuid_dup,
            .pci_bus_location = bus_dup,
            .pci_vendor_id = vendor,
            .pci_device_id = device_id,
            .vram_total_bytes = 0,
            .vram_used_bytes = 0,
            .core_clock_mhz = -1,
            .memory_clock_mhz = -1,
        };
    }
    closedir(dir);

    /* ---------------- best-effort memory/clocks, via nvidia-smi ---------------- */

    char *nvidia_smi = nullptr;
    {
        const char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            if (path_copy) {
                char *saveptr = nullptr;
                for (char *d = strtok_r(path_copy, ":", &saveptr); d;
                     d = strtok_r(nullptr, ":", &saveptr)) {
                    char candidate[4096];
                    int written = snprintf(candidate, sizeof candidate, "%s/nvidia-smi", d);
                    if (written < 0 || (size_t)written >= sizeof candidate) continue;
                    if (access(candidate, X_OK) == 0) {
                        nvidia_smi = strdup(candidate);
                        break;
                    }
                }
                free(path_copy);
            }
        }
    }

    if (!nvidia_smi) {
        data->nvidia_smi_available = false;
        return 0;
    }

    int pipefd[2];
    char *output = nullptr;
    if (pipe(pipefd) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) dup2(devnull, STDERR_FILENO);
            execl(nvidia_smi, nvidia_smi,
                  "--query-gpu=uuid,memory.total,memory.used,clocks.current.sm,"
                  "clocks.current.memory",
                  "--format=csv,noheader,nounits", nullptr);
            _exit(127);
        } else if (pid > 0) {
            close(pipefd[1]);
            size_t cap2 = 4096, len2 = 0;
            output = malloc(cap2);
            if (output) {
                for (;;) {
                    if (len2 + 1024 > cap2) {
                        cap2 *= 2;
                        char *grown = realloc(output, cap2);
                        if (!grown) {
                            free(output);
                            output = nullptr;
                            break;
                        }
                        output = grown;
                    }
                    ssize_t n = read(pipefd[0], output + len2, cap2 - len2 - 1);
                    if (n <= 0) break;
                    len2 += (size_t)n;
                }
                if (output) output[len2] = '\0';
            }
            close(pipefd[0]);
            int status;
            waitpid(pid, &status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                free(output);
                output = nullptr;
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
        }
    }
    free(nvidia_smi);

    data->nvidia_smi_available = output != nullptr;
    if (!output) return 0;

    char *cursor = output;
    while (*cursor) {
        char *line_start = cursor;
        char *newline = strchr(cursor, '\n');
        char *line_end = newline ? newline : line_start + strlen(line_start);
        cursor = newline ? newline + 1 : line_end;
        *line_end = '\0';
        if (line_start == line_end) continue;

        char *fields[5] = {0};
        int n = 0;
        char *tok = strtok(line_start, ",");
        while (tok && n < 5) {
            fields[n++] = tok;
            tok = strtok(nullptr, ",");
        }
        if (n < 5) continue;

        char *row_uuid = trim(fields[0]);
        for (int i = 0; i < data->device_count; i++) {
            if (strcmp(data->devices[i].uuid, row_uuid) == 0) {
                data->devices[i].vram_total_bytes =
                    strtoull(trim(fields[1]), nullptr, 10) * 1024ULL * 1024ULL;
                data->devices[i].vram_used_bytes =
                    strtoull(trim(fields[2]), nullptr, 10) * 1024ULL * 1024ULL;
                data->devices[i].core_clock_mhz = atoi(trim(fields[3]));
                data->devices[i].memory_clock_mhz = atoi(trim(fields[4]));
                break;
            }
        }
    }

    free(output);
    return 0;
}
