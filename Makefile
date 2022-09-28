

.PHONY: all clean

BOARD = src/arch/arm/tm4c123gh6pm

include makedefs

vpath %.c src/arch/arm/cpu-cm4f
vpath %.c src/arch/arm/tm4c123gh6pm
vpath %.c src/utils
vpath %.c src

srcs = startup.c board_init.c board_imp.c start.c misc_utils.c armv7m_utils.c kernel.c main.c
obj = $(subst .c,.o,$(srcs))

all: blink

clean:
	rm -f *.o
	rm -f blink

blink: $(obj)
	$(LINK.o) -T $(BOARD)/gnu-linker.ld $(obj) -o $@
	cscope -b -q -k

blink.bin: blink
	arm-elf-linux-gnueabi-objcopy -O binary $< $@


%.o: %.c
	$(COMPILE.c) -MMD -MP $< -o $@

deps = $(srcs:.c=.d)

-include $(deps)
