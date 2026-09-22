# Builds libgpu_intel_linux.so from src/gpu_intel/.
GPU_INTEL_SRCS := src/gpu_intel/gpu_intel.c src/gpu_intel/linux/libgpu_intel_linux.c
GPU_INTEL_INC  := -Iinclude -Isrc/gpu_intel
GPU_INTEL_OUT  := dist/intel/libgpu_intel_linux.so

$(GPU_INTEL_OUT): $(GPU_INTEL_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $(GPU_INTEL_INC) -o $@ $(GPU_INTEL_SRCS)

.PHONY: lib-gpu_intel
lib-gpu_intel: $(GPU_INTEL_OUT)

LIBS += $(GPU_INTEL_OUT)
