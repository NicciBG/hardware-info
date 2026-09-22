# Builds dist/nvidia/gpu_nvidia_probe, dynamically linked against
# dist/nvidia/libgpu_nvidia_linux.so with an $ORIGIN rpath so it finds it sitting
# next to itself -- no LD_LIBRARY_PATH needed on the tester's end.
GPU_NVIDIA_PROBE_OUT := dist/nvidia/gpu_nvidia_probe

$(GPU_NVIDIA_PROBE_OUT): tools/gpu_nvidia_probe.c dist/nvidia/libgpu_nvidia_linux.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gpu_nvidia_probe.c -L$(dir dist/nvidia/libgpu_nvidia_linux.so) -lgpu_nvidia_linux -Wl,-rpath,'$$ORIGIN'

.PHONY: probe-gpu_nvidia
probe-gpu_nvidia: $(GPU_NVIDIA_PROBE_OUT)

PROBES += $(GPU_NVIDIA_PROBE_OUT)
