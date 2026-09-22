# Builds and runs tests/test_hardware_info.c (sanitizer-enabled correctness check,
# not the shippable probe -- see hardware_info_probe.mk for that).
TEST_HARDWARE_INFO_SRCS := src/hardware_info/hardware_info.c src/hardware_info/linux/libhardware_info_linux.c tests/test_hardware_info.c
TEST_HARDWARE_INFO_INC  := -Iinclude -Isrc/hardware_info
TEST_HARDWARE_INFO_OUT  := build/bin/test_hardware_info

$(TEST_HARDWARE_INFO_OUT): $(TEST_HARDWARE_INFO_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(TESTFLAGS) $(TEST_HARDWARE_INFO_INC) -o $@ $(TEST_HARDWARE_INFO_SRCS)

.PHONY: run-test-hardware_info
run-test-hardware_info: $(TEST_HARDWARE_INFO_OUT)
	$(TEST_HARDWARE_INFO_OUT)

TESTS += run-test-hardware_info
