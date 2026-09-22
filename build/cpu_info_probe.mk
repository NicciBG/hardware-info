# Builds dist/cpu_info/cpu_info_probe, dynamically linked against
# dist/cpu_info/libcpu_info_linux.so with an $ORIGIN rpath so it finds it sitting
# next to itself -- no LD_LIBRARY_PATH needed on the tester's end.
CPU_INFO_PROBE_OUT := dist/cpu_info/cpu_info_probe

$(CPU_INFO_PROBE_OUT): tools/cpu_info_probe.c dist/cpu_info/libcpu_info_linux.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/cpu_info_probe.c -L$(dir dist/cpu_info/libcpu_info_linux.so) -lcpu_info_linux -Wl,-rpath,'$$ORIGIN'

.PHONY: probe-cpu_info
probe-cpu_info: $(CPU_INFO_PROBE_OUT)

PROBES += $(CPU_INFO_PROBE_OUT)
