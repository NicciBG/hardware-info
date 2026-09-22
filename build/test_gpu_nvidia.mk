# Builds and runs tests/test_gpu_nvidia.c (sanitizer-enabled correctness check,
# not the shippable probe -- see gpu_nvidia_probe.mk for that).
TEST_GPU_NVIDIA_SRCS := src/gpu_nvidia/gpu_nvidia.c src/gpu_nvidia/linux/libgpu_nvidia_linux.c tests/test_gpu_nvidia.c
TEST_GPU_NVIDIA_INC  := -Iinclude -Isrc/gpu_nvidia
TEST_GPU_NVIDIA_OUT  := build/bin/test_gpu_nvidia

$(TEST_GPU_NVIDIA_OUT): $(TEST_GPU_NVIDIA_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(TESTFLAGS) $(TEST_GPU_NVIDIA_INC) -o $@ $(TEST_GPU_NVIDIA_SRCS)

.PHONY: run-test-gpu_nvidia
run-test-gpu_nvidia: $(TEST_GPU_NVIDIA_OUT)
	$(TEST_GPU_NVIDIA_OUT)

TESTS += run-test-gpu_nvidia
