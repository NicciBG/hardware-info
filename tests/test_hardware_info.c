#include "../include/hardware_info.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check_true(const char *what, bool cond) {
    if (!cond) {
        failures++;
        printf("FAIL %s\n", what);
    } else {
        printf("ok   %s\n", what);
    }
}

static void check_int(const char *what, int64_t got, int64_t want) {
    check_true(what, got == want);
    if (got != want) printf("     got %lld, want %lld\n", (long long)got, (long long)want);
}

static const StorageDevice *find_device(const HARDWARE_DATA *d, const char *name) {
    for (int i = 0; i < d->storage_device_count; i++) {
        if (strcmp(d->storage_devices[i].device_name, name) == 0) return &d->storage_devices[i];
    }
    return nullptr;
}

int main(void) {
    HARDWARE_DATA data;
    int rc = get_hardware_data(&data);
    check_int("get_hardware_data succeeds", rc, 0);
    if (rc != 0) return 1;

    printf("  total_ram_kib=%llu available_ram_kib=%llu\n",
           (unsigned long long)data.total_ram_kib, (unsigned long long)data.available_ram_kib);
    check_int("total RAM (kB)", (int64_t)data.total_ram_kib, 65599440);
    check_true("available RAM > 0", data.available_ram_kib > 0);
    check_true("available RAM <= total RAM", data.available_ram_kib <= data.total_ram_kib);

    printf("  %d storage device(s):\n", data.storage_device_count);
    for (int i = 0; i < data.storage_device_count; i++) {
        StorageDevice *s = &data.storage_devices[i];
        printf("    %s: model=\"%s\" size=%.2f GB media=%d interface=%d driver=\"%s\"\n",
               s->device_name, s->model, (double)s->size_bytes / 1e9, s->media,
               s->interface_type, s->interface_driver);
    }
    check_true("loop devices excluded (virtual, not real storage)",
               find_device(&data, "loop0") == nullptr);

    const StorageDevice *nvme = find_device(&data, "nvme0n1");
    check_true("nvme0n1 found", nvme != nullptr);
    if (nvme) {
        check_true("nvme0n1 model contains 'Samsung'", strstr(nvme->model, "Samsung") != nullptr);
        check_int("nvme0n1 media is SSD", nvme->media, STORAGE_MEDIA_SSD);
        check_int("nvme0n1 interface is NVMe", nvme->interface_type, STORAGE_INTERFACE_NVME);
        check_true("nvme0n1 size ~1TB",
                   nvme->size_bytes > 900000000000ULL && nvme->size_bytes < 1100000000000ULL);
    }

    const StorageDevice *sda = find_device(&data, "sda");
    check_true("sda found", sda != nullptr);
    if (sda) {
        check_true("sda model contains 'WDC'", strstr(sda->model, "WDC") != nullptr);
        check_int("sda media is HDD", sda->media, STORAGE_MEDIA_HDD);
        check_int("sda interface is SATA (ahci)", sda->interface_type, STORAGE_INTERFACE_SATA);
        check_true("sda interface_driver is 'ahci'", strcmp(sda->interface_driver, "ahci") == 0);
        check_true("sda size ~6TB",
                   sda->size_bytes > 5500000000000ULL && sda->size_bytes < 6500000000000ULL);
    }

    const StorageDevice *sdb = find_device(&data, "sdb");
    check_true("sdb found", sdb != nullptr);
    if (sdb) {
        check_int("sdb media is SSD", sdb->media, STORAGE_MEDIA_SSD);
        check_int("sdb interface is SATA (ahci)", sdb->interface_type, STORAGE_INTERFACE_SATA);
    }

    free_hardware_data(&data);
    check_true("storage_devices null after free", data.storage_devices == nullptr);
    check_int("storage_device_count 0 after free", data.storage_device_count, 0);

    printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) failed.\n", failures);
    return failures == 0 ? 0 : 1;
}
