# Builds dist/hardware_info/hardware_info_probe, dynamically linked against
# dist/hardware_info/libhardware_info_linux.so with an $ORIGIN rpath so it finds it sitting
# next to itself -- no LD_LIBRARY_PATH needed on the tester's end.
HARDWARE_INFO_PROBE_OUT := dist/hardware_info/hardware_info_probe

$(HARDWARE_INFO_PROBE_OUT): tools/hardware_info_probe.c dist/hardware_info/libhardware_info_linux.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/hardware_info_probe.c -L$(dir dist/hardware_info/libhardware_info_linux.so) -lhardware_info_linux -Wl,-rpath,'$$ORIGIN'

.PHONY: probe-hardware_info
probe-hardware_info: $(HARDWARE_INFO_PROBE_OUT)

PROBES += $(HARDWARE_INFO_PROBE_OUT)
