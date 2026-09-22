# Builds dist/qualcomm/gpu_qualcomm_probe, dynamically linked against
# dist/qualcomm/libgpu_qualcomm_linux.so with an $ORIGIN rpath so it finds it sitting
# next to itself -- no LD_LIBRARY_PATH needed on the tester's end.
GPU_QUALCOMM_PROBE_OUT := dist/qualcomm/gpu_qualcomm_probe

$(GPU_QUALCOMM_PROBE_OUT): tools/gpu_qualcomm_probe.c dist/qualcomm/libgpu_qualcomm_linux.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gpu_qualcomm_probe.c -L$(dir dist/qualcomm/libgpu_qualcomm_linux.so) -lgpu_qualcomm_linux -Wl,-rpath,'$$ORIGIN'

.PHONY: probe-gpu_qualcomm
probe-gpu_qualcomm: $(GPU_QUALCOMM_PROBE_OUT)

PROBES += $(GPU_QUALCOMM_PROBE_OUT)
