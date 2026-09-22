# Builds libgpu_qualcomm_linux.so from src/gpu_qualcomm/.
GPU_QUALCOMM_SRCS := src/gpu_qualcomm/gpu_qualcomm.c src/gpu_qualcomm/linux/libgpu_qualcomm_linux.c
GPU_QUALCOMM_INC  := -Iinclude -Isrc/gpu_qualcomm
GPU_QUALCOMM_OUT  := dist/qualcomm/libgpu_qualcomm_linux.so

$(GPU_QUALCOMM_OUT): $(GPU_QUALCOMM_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $(GPU_QUALCOMM_INC) -o $@ $(GPU_QUALCOMM_SRCS)

.PHONY: lib-gpu_qualcomm
lib-gpu_qualcomm: $(GPU_QUALCOMM_OUT)

LIBS += $(GPU_QUALCOMM_OUT)
