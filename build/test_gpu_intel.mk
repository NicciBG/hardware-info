# Builds and runs tests/test_gpu_intel.c (sanitizer-enabled correctness check,
# not the shippable probe -- see gpu_intel_probe.mk for that).
TEST_GPU_INTEL_SRCS := src/gpu_intel/gpu_intel.c src/gpu_intel/linux/libgpu_intel_linux.c tests/test_gpu_intel.c
TEST_GPU_INTEL_INC  := -Iinclude -Isrc/gpu_intel
TEST_GPU_INTEL_OUT  := build/bin/test_gpu_intel

$(TEST_GPU_INTEL_OUT): $(TEST_GPU_INTEL_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(TESTFLAGS) $(TEST_GPU_INTEL_INC) -o $@ $(TEST_GPU_INTEL_SRCS)

.PHONY: run-test-gpu_intel
run-test-gpu_intel: $(TEST_GPU_INTEL_OUT)
	$(TEST_GPU_INTEL_OUT)

TESTS += run-test-gpu_intel
