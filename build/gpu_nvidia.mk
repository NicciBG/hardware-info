# Builds libgpu_nvidia_linux.so from src/gpu_nvidia/.
GPU_NVIDIA_SRCS := src/gpu_nvidia/gpu_nvidia.c src/gpu_nvidia/linux/libgpu_nvidia_linux.c
GPU_NVIDIA_INC  := -Iinclude -Isrc/gpu_nvidia
GPU_NVIDIA_OUT  := dist/nvidia/libgpu_nvidia_linux.so

$(GPU_NVIDIA_OUT): $(GPU_NVIDIA_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $(GPU_NVIDIA_INC) -o $@ $(GPU_NVIDIA_SRCS)

.PHONY: lib-gpu_nvidia
lib-gpu_nvidia: $(GPU_NVIDIA_OUT)

LIBS += $(GPU_NVIDIA_OUT)
