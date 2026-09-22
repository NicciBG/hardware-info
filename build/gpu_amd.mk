# Builds libgpu_amd_linux.so from src/gpu_amd/.
GPU_AMD_SRCS := src/gpu_amd/gpu_amd.c src/gpu_amd/linux/libgpu_amd_linux.c
GPU_AMD_INC  := -Iinclude -Isrc/gpu_amd
GPU_AMD_OUT  := dist/amd/libgpu_amd_linux.so

$(GPU_AMD_OUT): $(GPU_AMD_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $(GPU_AMD_INC) -o $@ $(GPU_AMD_SRCS)

.PHONY: lib-gpu_amd
lib-gpu_amd: $(GPU_AMD_OUT)

LIBS += $(GPU_AMD_OUT)
