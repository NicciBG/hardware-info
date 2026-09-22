#define _XOPEN_SOURCE 700 /* realpath() */

#include "hardware_info_platform.h"

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

static uint64_t read_sysfs_u64(const char *path, uint64_t default_value) {
    char buf[64];
    if (!read_sysfs_line(path, buf, sizeof buf)) return default_value;
    return strtoull(buf, nullptr, 10);
}

static char *rtrim_dup(const char *s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    char *out = malloc(len + 1);
    if (!out) return nullptr;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

/* ---------------- RAM ---------------- */

static bool read_meminfo_value(const char *key, uint64_t *out) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return false;
    char line[256];
    size_t key_len = strlen(key);
    bool found = false;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, key, key_len) == 0) {
            unsigned long long value = 0;
            if (sscanf(line + key_len, "%llu", &value) == 1) {
                *out = (uint64_t)value;
                found = true;
            }
            break;
        }
    }
    fclose(f);
    return found;
}

int platform_get_ram_info(HARDWARE_DATA *data) {
    uint64_t total = 0, avail = 0;
    if (!read_meminfo_value("MemTotal:", &total)) return 202;
    if (!read_meminfo_value("MemAvailable:", &avail)) return 202;
    data->total_ram_kib = total;
    data->available_ram_kib = avail;
    return 0;
}

/* ------------- Storage devices ------------- */

static StorageInterface classify_driver(const char *driver) {
    if (!driver || !*driver) return STORAGE_INTERFACE_UNKNOWN;
    if (strcmp(driver, "ahci") == 0 || strncmp(driver, "sata_", 5) == 0)
        return STORAGE_INTERFACE_SATA;
    if (strncmp(driver, "pata_", 5) == 0 || strcmp(driver, "ata_piix") == 0)
        return STORAGE_INTERFACE_PATA;
    if (strcmp(driver, "usb-storage") == 0 || strcmp(driver, "uas") == 0)
        return STORAGE_INTERFACE_USB;
    if (strcmp(driver, "mpt3sas") == 0 || strcmp(driver, "mpt2sas") == 0 ||
        strcmp(driver, "megaraid_sas") == 0 || strcmp(driver, "hpsa") == 0 ||
        strcmp(driver, "aacraid") == 0 || strcmp(driver, "isci") == 0)
        return STORAGE_INTERFACE_SAS;
    if (strcmp(driver, "virtio_scsi") == 0 || strcmp(driver, "virtio_blk") == 0)
        return STORAGE_INTERFACE_VIRTIO;
    if (strstr(driver, "sbp2") || strstr(driver, "firewire")) return STORAGE_INTERFACE_FIREWIRE;
    return STORAGE_INTERFACE_UNKNOWN;
}

static int extract_host_number(const char *path) {
    const char *p = strstr(path, "/host");
    if (!p) return -1;
    p += 5;
    if (!isdigit((unsigned char)*p)) return -1;
    return atoi(p);
}

static char *read_scsi_host_driver(int host_num) {
    char path[128];
    snprintf(path, sizeof path, "/sys/class/scsi_host/host%d/proc_name", host_num);
    char buf[128];
    if (!read_sysfs_line(path, buf, sizeof buf)) return nullptr;
    return strdup(buf);
}

int platform_get_storage_devices(HARDWARE_DATA *data) {
    DIR *dir = opendir("/sys/block");
    if (!dir) return 202;

    size_t cap = 8;
    data->storage_devices = malloc(cap * sizeof(StorageDevice));
    data->storage_device_count = 0;
    if (!data->storage_devices) {
        closedir(dir);
        return 203;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char block_path[320];
        snprintf(block_path, sizeof block_path, "/sys/block/%s", entry->d_name);

        char *real = realpath(block_path, nullptr);
        if (!real) continue;

        if (strstr(real, "/virtual/")) {
            free(real);
            continue;
        }

        char sub[400];
        snprintf(sub, sizeof sub, "%s/size", block_path);
        uint64_t sectors = read_sysfs_u64(sub, 0);
        if (sectors == 0) {
            free(real);
            continue;
        }

        snprintf(sub, sizeof sub, "%s/device/type", block_path);
        char type_buf[8] = {0};
        if (read_sysfs_line(sub, type_buf, sizeof type_buf) && strcmp(type_buf, "5") == 0) {
            free(real); /* optical drive, not an HDD/SSD */
            continue;
        }

        snprintf(sub, sizeof sub, "%s/queue/rotational", block_path);
        char rot_buf[8] = {0};
        StorageMedia media = STORAGE_MEDIA_UNKNOWN;
        if (read_sysfs_line(sub, rot_buf, sizeof rot_buf)) {
            media = strcmp(rot_buf, "1") == 0   ? STORAGE_MEDIA_HDD
                    : strcmp(rot_buf, "0") == 0 ? STORAGE_MEDIA_SSD
                                                 : STORAGE_MEDIA_UNKNOWN;
        }

        snprintf(sub, sizeof sub, "%s/device/model", block_path);
        char model_buf[128] = {0};
        read_sysfs_line(sub, model_buf, sizeof model_buf);
        char *model = rtrim_dup(model_buf);

        StorageInterface iface;
        char *driver;
        if (strstr(real, "/nvme/")) {
            iface = STORAGE_INTERFACE_NVME;
            driver = strdup("nvme");
        } else if (strncmp(entry->d_name, "mmcblk", 6) == 0 || strstr(real, "/mmc_host/")) {
            iface = STORAGE_INTERFACE_MMC;
            driver = strdup("mmc");
        } else {
            int host_num = extract_host_number(real);
            driver = host_num >= 0 ? read_scsi_host_driver(host_num) : nullptr;
            iface = classify_driver(driver);
            if (!driver) driver = strdup("");
        }
        free(real);

        if (!model || !driver) {
            free(model);
            free(driver);
            closedir(dir);
            return 203;
        }

        if ((size_t)data->storage_device_count >= cap) {
            cap *= 2;
            StorageDevice *grown = realloc(data->storage_devices, cap * sizeof(StorageDevice));
            if (!grown) {
                free(model);
                free(driver);
                closedir(dir);
                return 203;
            }
            data->storage_devices = grown;
        }

        data->storage_devices[data->storage_device_count++] = (StorageDevice){
            .device_name = strdup(entry->d_name),
            .model = model,
            .size_bytes = sectors * 512,
            .media = media,
            .interface_type = iface,
            .interface_driver = driver,
        };
    }

    closedir(dir);
    return 0;
}
