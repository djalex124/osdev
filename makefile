build_speed = -O2

gcc = x86_64-elf-gcc

kernel_flags = -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -DAQUA_VER_BUILD=$$(cat build.txt) -gdwarf-5 -fno-pie -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall
kernel_link  = -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -gdwarf-5 -fno-pie -T bin/link.ld

all: run

boot_c := $(wildcard src/x86_64/*.c)
boot_s := $(wildcard src/x86_64/*.S)

boot_obj_c = $(boot_c:.c=.o)
boot_obj_s = $(boot_s:.S=.o)

kernel_c := $(wildcard src/kernel/*.c)
kernel_s := $(wildcard src/kernel/*.S)

kernel_obj_c = $(kernel_c:.c=.o)
kernel_obj_s = $(kernel_s:.S=.o)

build_obj := $(boot_obj_c) $(boot_obj_s) $(kernel_obj_c) $(kernel_obj_s)
all_obj := $(build_obj)

bin/link.ld:
	@$(gcc) -E -P -x c $(kernel_flags) src/x86_64/link.ld >bin/link.ld

bin/kernel.bin: $(all_obj) bin/link.ld
	@$(gcc) $(kernel_link) $(all_obj) -o bin/kernel.bin -nostdlib -lgcc
	@objcopy --strip-debug bin/kernel.bin

bin/dbg_kernel.bin: $(all_obj) bin/link.ld
	@$(gcc) $(kernel_link) $(all_obj) -o bin/dbg_kernel.bin -nostdlib -lgcc

%.o: %.c 
	@$(gcc) $(kernel_flags) -c -MMD -MP $< -o $@ -lgcc

%.o: %.S 
	@$(gcc) $(kernel_flags) -c -DASSEMBLY -MMD -MP $< -o $@ -lgcc

bin/boot.iso: bin/kernel.bin
	@cp bin/kernel.bin grub/boot/kernel.bin
	@grub-mkrescue -o bin/boot.iso grub
	@echo $$(($$(cat build.txt) + 1)) > build.txt

bin/dbg_boot.iso: bin/dbg_kernel.bin
	@cp bin/dbg_kernel.bin grub/boot/kernel.bin
	@grub-mkrescue -o bin/boot.iso grub
	@echo $$(($$(cat build.txt) + 1)) > build.txt

run: bin/boot.iso
	@qemu-system-x86_64 -m 2048 -cdrom bin/boot.iso -net none

debug: kernel_flags += -DAQUA_DEBUG
debug: bin/dbg_boot.iso
	@qemu-system-x86_64 -m 2048 -cdrom bin/boot.iso -net none -s -d int

clean:
	@rm -f bin/link.ld
	@rm -f $(build_obj)
	@rm -f bin/kernel.bin bin/dbg_kernel.bin
	@rm -f bin/boot.iso