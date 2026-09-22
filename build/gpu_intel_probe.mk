# Builds dist/intel/gpu_intel_probe, dynamically linked against
# dist/intel/libgpu_intel_linux.so with an $ORIGIN rpath so it finds it sitting
# next to itself -- no LD_LIBRARY_PATH needed on the tester's end.
GPU_INTEL_PROBE_OUT := dist/intel/gpu_intel_probe

$(GPU_INTEL_PROBE_OUT): tools/gpu_intel_probe.c dist/intel/libgpu_intel_linux.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gpu_intel_probe.c -L$(dir dist/intel/libgpu_intel_linux.so) -lgpu_intel_linux -Wl,-rpath,'$$ORIGIN'

.PHONY: probe-gpu_intel
probe-gpu_intel: $(GPU_INTEL_PROBE_OUT)

PROBES += $(GPU_INTEL_PROBE_OUT)
