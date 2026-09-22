# Computer_info

Small C23 libraries that read what a machine's hardware actually is —
CPU topology and instruction-set support, RAM and storage devices, and
per-vendor GPU info — for use across other projects. No third-party
dependencies: each library reads directly from what the OS already
exposes (CPUID, `/proc`, `/sys`) or, where nothing else exists, from a
vendor CLI tool that ships with its own driver (`nvidia-smi`).

This replaces the earlier version of this repo. The data layout is
carried over from that version with two corrections (see
[`src/cpu_info/`](#cpu_info)); everything else here is new.

## Layout

Each library is independent — its own header, its own `.c` files, its
own platform split:

```
include/<library>.h              public API + struct definitions
src/<library>/<library>.c        shared logic (thin — usually just
                                  orchestration; see cpu_info for the
                                  one case with real shared logic)
src/<library>/<library>_platform.h   internal seam, not public
src/<library>/linux/lib<library>_linux.c
src/<library>/windows/<library>_windows.c
src/<library>/macos/lib<library>_macos.c
tests/test_<library>.c           automated, sanitizer-checked correctness tests
tools/<library>_probe.c          standalone .txt-dumping verification
                                  executable, for hardware not available
                                  to test with directly
build/<library>.mk                one file per library: builds the .so
build/test_<library>.mk           one file per test: builds + runs it
build/<library>_probe.mk          one file per probe: builds it into dist/
Makefile                          root — includes build/*.mk, defines
                                  `all`/`test`/`clean`
```

A file with no code in it means that platform isn't implemented yet —
see the [support matrix](#platform-support) below for exactly what's
real versus a placeholder.

## Building

`make all` builds every library and every probe executable into `dist/`;
`make test` builds and runs every automated test in `build/bin/`;
`make clean` removes both. `CC` and `CFLAGS` can be overridden the usual
way (`make CC=clang all`).

The `build/` directory holds the actual build logic — one `.mk` fragment
per library (`build/<library>.mk`), one per automated test
(`build/test_<library>.mk`), and one per probe executable
(`build/<library>_probe.mk`), each self-contained (no cross-file
variable dependencies, so include order doesn't matter) and included by
the root `Makefile`. `make lib-cpu_info`, `make run-test-gpu_intel`,
`make probe-gpu_amd`, etc. build just one target if you don't want
everything.

To build a single library by hand instead, every one follows the same
pattern — substitute the library name and platform:

```sh
# as a shared library
gcc -std=c23 -O2 -fPIC -shared \
  -Iinclude -Isrc/<library> \
  -o lib<library>_linux.so \
  src/<library>/<library>.c src/<library>/linux/lib<library>_linux.c

# or straight into your own program, no shared library at all
gcc -std=c23 -Iinclude -Isrc/<library> \
  -o myapp myapp.c src/<library>/<library>.c src/<library>/linux/lib<library>_linux.c
```

`cpu_info` and `hardware_info` don't take the `lib`/no-`lib` naming split
Windows needs — only the GPU libraries do, since only those were built
with an eventual multi-vendor DLL naming scheme in mind
(`libgpu_amd_linux.so`, `gpu_amd_windows.dll`, `libgpu_amd_macos.dylib`,
same for intel/nvidia/qualcomm).

## The common pattern

Every library exposes exactly two public functions, both `DLL_EXPORT`:

```c
int  get_<library>_data(<LIBRARY>_DATA *data);   /* fills data, 0 = success */
void free_<library>_data(<LIBRARY>_DATA *data);  /* releases everything get_ allocated */
```

Usage is always:

```c
CPU_DATA data;
int rc = get_cpu_data(&data);
if (rc != 0) {
    /* see the error-code block at the bottom of the relevant header */
} else {
    /* use data.* */
    free_cpu_data(&data);
}
```

**Always free with the library's own `free_*` function, never by hand.**
Across a shared-library boundary the caller and the library aren't
guaranteed to share a CRT heap (this matters concretely on Windows) —
freeing with the same allocator that made the allocation is the whole
reason these functions are exported at all rather than left as an
exercise for the caller.

A `device_count` (or, for CPU, just the struct filling in) of `0` is not
an error — it means the return code was `0` but nothing of that kind was
found on this machine. Check the return code for actual failure, not
whether anything was found.

## Libraries

### cpu_info

CPU brand/vendor and instruction-set flags via CPUID (`cpu_cpuid`, inline
`cpuid` asm on GCC/Clang, `__cpuidex` on MSVC — identical logic on every
OS, in `cpu_info.c`); core topology and cache sizes are OS-specific
(`libcpu_info_linux.c` etc.).

**Carried over from the previous version of this repo, with two bugs
fixed while porting:**
1. L1 cache size was read into one field regardless of whether the
   sysfs entry being read was the data or instruction cache — since both
   report as "level 1" and instruction was processed second, the stored
   value ended up being the instruction cache size. Split into
   `l1data`/`l1instruction`, each read from its own correctly-typed
   sysfs entry.
2. Cache info was read once from logical core 0 and assumed to apply to
   every core — wrong on any CPU with mixed core types (e.g. Intel's
   performance/efficiency hybrid design), where L1 and L2 sizes
   genuinely differ between core types. Now read per logical core.

```c
typedef struct {
    char *cpu_name;                 /* brand string, e.g. "13th Gen Intel(R) Core(TM) i5-13600K" */
    int   logical_core_count;
    int   physical_core_count;
    PhysicalCoreInfo *cores;        /* length = physical_core_count */

    int *l1data;                    /* per logical core, KiB */
    int *l1instruction;             /* per logical core, KiB */
    int *frequency;                 /* per logical core, current MHz */

    L2_Cache_Group *l2_groups;      /* L2 is shared across a cluster of
                                        cores, not owned per-core — see below */
    int l2_group_count;

    int l3size;                     /* KiB, single shared value */

    CPU_Algorithms algorithms;      /* SSE.. AVX2, BMI1/2, AVX512F, SHA,
                                        plus AMD-only (SSE4A, XOP, FMA4,
                                        3DNow!+) and Intel-only (POPCNT,
                                        AES, FMA, F16C, RDRAND, ...) flags */
} CPU_DATA;
```

`PhysicalCoreInfo` — one entry per physical core:
```c
typedef struct {
    int id;
    CoreType type;        /* CORE_TYPE_PERFORMANCE / _EFFICIENCY / _UNKNOWN */
    int *logical_ids;      /* which logical cores belong to this physical core */
    int logical_count;
} PhysicalCoreInfo;
```

`L2_Cache_Group` — one entry per *distinct* L2 instance, not per core.
On a hybrid CPU a performance core's two hyperthreads might share one
2MB L2 while a cluster of four efficiency cores shares one 4MB L2 — this
is why L2 isn't just another per-logical-core array like L1:
```c
typedef struct {
    int size_kib;
    int *logical_ids;      /* every logical core that shares this L2 instance */
    int logical_count;
} L2_Cache_Group;
```

```c
CPU_DATA data;
if (get_cpu_data(&data) == 0) {
    printf("%s: %d physical / %d logical cores, %d KiB L3\n",
           data.cpu_name, data.physical_core_count,
           data.logical_core_count, data.l3size);
    if (data.algorithms.AVX2) printf("AVX2 available\n");
    free_cpu_data(&data);
}
```

### hardware_info

RAM and storage devices — the non-CPU, non-GPU half of hardware
detection. Unlike CPUID, there's no OS-independent mechanism for either
of these, so almost all of the logic is platform-specific.

```c
typedef struct {
    uint64_t total_ram_kib;
    uint64_t available_ram_kib;     /* usable without swapping, not just "free" */

    StorageDevice *storage_devices; /* physical HDDs/SSDs only — virtual
                                        devices (loop, dm, ram/zram) and
                                        optical drives are filtered out */
    int storage_device_count;
} HARDWARE_DATA;

typedef struct {
    char *device_name;              /* "nvme0n1", "sda", ... */
    char *model;
    uint64_t size_bytes;
    StorageMedia media;             /* STORAGE_MEDIA_HDD / _SSD / _UNKNOWN */
    StorageInterface interface_type; /* _NVME / _SATA / _PATA / _SAS / _USB
                                         / _MMC / _VIRTIO / _FIREWIRE / _UNKNOWN */
    char *interface_driver;         /* raw driver name backing interface_type,
                                        e.g. "ahci" — always populated, most
                                        useful when interface_type is _UNKNOWN */
} StorageDevice;
```

Interface detection isn't a simple path-substring check — most of it
comes from tracing a device back to its SCSI host controller and reading
that controller's actual driver name (`ahci`, `usb-storage`,
`mpt3sas`, ...), which is what correctly tells SATA and PATA apart even
though Linux's `libata` subsystem exposes both under the same `ataN`
path shape.

```c
HARDWARE_DATA data;
if (get_hardware_data(&data) == 0) {
    printf("%llu KiB RAM total\n", (unsigned long long)data.total_ram_kib);
    for (int i = 0; i < data.storage_device_count; i++) {
        StorageDevice *d = &data.storage_devices[i];
        printf("%s: %s, %.1f GB\n", d->device_name, d->model,
               (double)d->size_bytes / 1e9);
    }
    free_hardware_data(&data);
}
```

### GPU libraries

One library per vendor rather than one generic GPU library, because the
actual detection mechanism is different enough between vendors that
sharing an implementation wouldn't save anything — a common struct
would just be padded with fields that are meaningless for one vendor or
another. Integrated and discrete GPUs from the *same* vendor, on the
other hand, don't get separate libraries — AMD's `amdgpu` driver and
Intel's `i915`/`xe` drivers both expose integrated and discrete parts
through the identical sysfs shape, so splitting by discrete/integrated
would have been a distinction without a difference. Apple and generic
mobile GPU vendors (ARM Mali, Imagination PowerVR) aren't covered.

#### gpu_amd

Reads AMD's own documented `amdgpu` sysfs interface
(`docs.kernel.org/gpu/amdgpu`).

```c
typedef struct {
    char *name;                 /* product_name if present, else a
                                    PCI-ID-based fallback */
    unsigned int pci_vendor_id;
    unsigned int pci_device_id;
    uint64_t vram_total_bytes;
    uint64_t vram_used_bytes;
    uint64_t gtt_total_bytes;   /* graphics translation table memory —
                                    particularly relevant on APUs, whose
                                    "VRAM" is actually a BIOS-carved-out
                                    slice of system RAM */
    int engine_clock_mhz;       /* current sclk */
    int memory_clock_mhz;       /* current mclk */
} GPU_AMD_Device;
```

#### gpu_intel

Reads Intel's `i915`/`xe` DRM sysfs interface. No VRAM field: confirmed
absent for integrated GPUs (no dedicated memory to report); untested
against a discrete Arc card, which may expose memory differently.

```c
typedef struct {
    char *name;                 /* i915 has no product-name file, so this
                                    is always a PCI-ID-based fallback */
    unsigned int pci_vendor_id;
    unsigned int pci_device_id;
    int max_freq_mhz;           /* RP0 */
    int min_freq_mhz;           /* RPn */
    int current_freq_mhz;       /* can legitimately read 0 when idle/power-gated */
} GPU_INTEL_Device;
```

#### gpu_nvidia

Identification comes from the proprietary driver's own
`/proc/driver/nvidia/gpus/<bus>/information` (present whenever that
driver is loaded, no extra package needed). Memory and clocks are
best-effort, via the `nvidia-smi` CLI the driver package installs
alongside itself — not NVML, since linking that would mean depending on
the CUDA toolkit's dev headers rather than just a header file. The
open-source `nouveau` driver isn't supported — it doesn't populate
`/proc/driver/nvidia/` at all.

```c
typedef struct {
    char *name;
    char *uuid;
    char *pci_bus_location;      /* e.g. "0000:01:00.0" */
    unsigned int pci_vendor_id;
    unsigned int pci_device_id;

    /* best-effort, via nvidia-smi — see nvidia_smi_available */
    uint64_t vram_total_bytes;
    uint64_t vram_used_bytes;
    int core_clock_mhz;
    int memory_clock_mhz;
} GPU_NVIDIA_Device;

typedef struct {
    GPU_NVIDIA_Device *devices;
    int device_count;
    bool nvidia_smi_available;   /* false if nvidia-smi couldn't be found —
                                     identification still works either way */
} GPU_NVIDIA_DATA;
```

#### gpu_qualcomm

Reads the `kgsl` kernel driver's sysfs interface
(`/sys/class/kgsl/kgsl-3d*`). Written directly against the driver's
source (`github.com/qualcomm-linux/kgsl`) to confirm attribute names and
formats — firmer ground than a guess, but **this is the one library in
this repo with no hardware to verify it against at all**, not even the
"nothing found" path. Treat it as a first draft.

```c
typedef struct {
    char *name;                 /* gpu_model, e.g. "Adreno730v2" — raw
                                    driver string */
    int current_clock_mhz;      /* gpuclk, converted from the driver's Hz */
    int max_clock_mhz;          /* max_gpuclk, converted from the driver's Hz */
    int busy_percent;           /* -1 if unavailable or GPU idle (total was 0) */
} GPU_QUALCOMM_Device;
```

## Verification tools

`tools/<library>_probe.c` builds into a small standalone executable that
calls the library, writes everything it found into a plain `.txt` file
in the working directory, and prints where it wrote it — meant to be
handed to someone with hardware you don't have, so their result can be
checked against what they actually have. `make all` builds every one of
these straight into its own `dist/<name>/` folder alongside the `.so` it
needs and a `README.md` with run instructions and — specific to that
library — exactly what hasn't been verified yet. Each is linked with an
`$ORIGIN` rpath, so the two files just need to stay in the same folder;
no environment setup needed on the tester's end. Building one by hand
instead of via `make`:

```sh
gcc -std=c23 -O2 -fPIC -shared -Iinclude -Isrc/gpu_amd \
  -o libgpu_amd_linux.so src/gpu_amd/gpu_amd.c src/gpu_amd/linux/libgpu_amd_linux.c
gcc -std=c23 -O2 -Iinclude -o gpu_amd_probe tools/gpu_amd_probe.c \
  -L. -lgpu_amd_linux -Wl,-rpath,'$ORIGIN'
```

## Platform support

| | Linux | Windows | macOS |
|---|---|---|---|
| cpu_info | done, verified on a real hybrid CPU | not implemented | not implemented |
| hardware_info | done, verified | not implemented | not implemented |
| gpu_amd | done, unverified — no AMD GPU available anywhere this was built | not implemented | not implemented |
| gpu_intel | done, verified on integrated UHD 770; discrete Arc/`xe` path unverified | not implemented | not implemented |
| gpu_nvidia | done, verified on a single RTX 3080; multi-GPU path unverified | not implemented | not implemented |
| gpu_qualcomm | done, entirely unverified — no Adreno hardware available at all | not implemented | not implemented |

"Not implemented" means exactly that — an empty `.c` file exists at the
expected path so the naming/build convention is already in place, but
there's no code in it yet.

## License

Unlicense (public domain) — see [LICENSE](LICENSE), unchanged from the
previous version of this repo.
