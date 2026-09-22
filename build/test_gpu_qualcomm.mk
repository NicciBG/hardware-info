# Builds and runs tests/test_gpu_qualcomm.c (sanitizer-enabled correctness check,
# not the shippable probe -- see gpu_qualcomm_probe.mk for that).
TEST_GPU_QUALCOMM_SRCS := src/gpu_qualcomm/gpu_qualcomm.c src/gpu_qualcomm/linux/libgpu_qualcomm_linux.c tests/test_gpu_qualcomm.c
TEST_GPU_QUALCOMM_INC  := -Iinclude -Isrc/gpu_qualcomm
TEST_GPU_QUALCOMM_OUT  := build/bin/test_gpu_qualcomm

$(TEST_GPU_QUALCOMM_OUT): $(TEST_GPU_QUALCOMM_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(TESTFLAGS) $(TEST_GPU_QUALCOMM_INC) -o $@ $(TEST_GPU_QUALCOMM_SRCS)

.PHONY: run-test-gpu_qualcomm
run-test-gpu_qualcomm: $(TEST_GPU_QUALCOMM_OUT)
	$(TEST_GPU_QUALCOMM_OUT)

TESTS += run-test-gpu_qualcomm
