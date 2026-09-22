# Builds libcpu_info_linux.so from src/cpu_info/.
CPU_INFO_SRCS := src/cpu_info/cpu_info.c src/cpu_info/linux/libcpu_info_linux.c
CPU_INFO_INC  := -Iinclude -Isrc/cpu_info
CPU_INFO_OUT  := dist/cpu_info/libcpu_info_linux.so

$(CPU_INFO_OUT): $(CPU_INFO_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $(CPU_INFO_INC) -o $@ $(CPU_INFO_SRCS)

.PHONY: lib-cpu_info
lib-cpu_info: $(CPU_INFO_OUT)

LIBS += $(CPU_INFO_OUT)
