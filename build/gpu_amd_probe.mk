# Builds dist/amd/gpu_amd_probe, dynamically linked against
# dist/amd/libgpu_amd_linux.so with an $ORIGIN rpath so it finds it sitting
# next to itself -- no LD_LIBRARY_PATH needed on the tester's end.
GPU_AMD_PROBE_OUT := dist/amd/gpu_amd_probe

$(GPU_AMD_PROBE_OUT): tools/gpu_amd_probe.c dist/amd/libgpu_amd_linux.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gpu_amd_probe.c -L$(dir dist/amd/libgpu_amd_linux.so) -lgpu_amd_linux -Wl,-rpath,'$$ORIGIN'

.PHONY: probe-gpu_amd
probe-gpu_amd: $(GPU_AMD_PROBE_OUT)

PROBES += $(GPU_AMD_PROBE_OUT)
