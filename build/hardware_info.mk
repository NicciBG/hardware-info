# Builds libhardware_info_linux.so from src/hardware_info/.
HARDWARE_INFO_SRCS := src/hardware_info/hardware_info.c src/hardware_info/linux/libhardware_info_linux.c
HARDWARE_INFO_INC  := -Iinclude -Isrc/hardware_info
HARDWARE_INFO_OUT  := dist/hardware_info/libhardware_info_linux.so

$(HARDWARE_INFO_OUT): $(HARDWARE_INFO_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $(HARDWARE_INFO_INC) -o $@ $(HARDWARE_INFO_SRCS)

.PHONY: lib-hardware_info
lib-hardware_info: $(HARDWARE_INFO_OUT)

LIBS += $(HARDWARE_INFO_OUT)
