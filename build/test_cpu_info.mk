# Builds and runs tests/test_cpu_info.c (sanitizer-enabled correctness check,
# not the shippable probe -- see cpu_info_probe.mk for that).
TEST_CPU_INFO_SRCS := src/cpu_info/cpu_info.c src/cpu_info/linux/libcpu_info_linux.c tests/test_cpu_info.c
TEST_CPU_INFO_INC  := -Iinclude -Isrc/cpu_info
TEST_CPU_INFO_OUT  := build/bin/test_cpu_info

$(TEST_CPU_INFO_OUT): $(TEST_CPU_INFO_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(TESTFLAGS) $(TEST_CPU_INFO_INC) -o $@ $(TEST_CPU_INFO_SRCS)

.PHONY: run-test-cpu_info
run-test-cpu_info: $(TEST_CPU_INFO_OUT)
	$(TEST_CPU_INFO_OUT)

TESTS += run-test-cpu_info
