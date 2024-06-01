build_speed:= -O2

kernel_flags := -g -ggdb -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -c
kernel_link  := -g -ggdb -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -T src/x86_64/link.ld

all: run

boot_c := $(wildcard src/x86_64/*.c)
boot_s := $(wildcard src/x86_64/*.S)

boot_obj_c = $(boot_c:.c=.o)
boot_obj_s = $(boot_s:.S=.o)

kernel_c := $(wildcard src/kernel/*.c)
kernel_s := $(wildcard src/kernel/*.S)

kernel_obj_c = $(kernel_c:.c=.o)
kernel_obj_s = $(kernel_s:.S=.o)

all_obj := $(boot_obj_c) $(boot_obj_s) $(kernel_obj_c) $(kernel_obj_s)

bin/kernel.bin: $(all_obj)
	/home/alexk/ccx86_64/bin/x86_64-elf-gcc $(kernel_link) $(all_obj) -o bin/kernel.bin -nostdlib -lgcc
	@nm bin/kernel.bin > bin/kernel.map
	@cp bin/kernel.bin bin/dbg_kernel.bin
	@objcopy --strip-debug bin/kernel.bin

%.o: %.c 
	@/home/alexk/ccx86_64/bin/x86_64-elf-gcc $(kernel_flags) -MMD -MP $< -o $@ -lgcc

%.o: %.S 
	@/home/alexk/ccx86_64/bin/x86_64-elf-gcc $(kernel_flags) -DASSEMBLY -MMD -MP $< -o $@ -lgcc

bin/boot.iso: bin/kernel.bin
	@cp bin/kernel.bin grub/boot/kernel.bin
	@grub-mkrescue -o bin/boot.iso grub

run: bin/boot.iso
	@qemu-system-x86_64 -m 2048 -cdrom bin/boot.iso -net none

debug: bin/boot.iso
	@qemu-system-x86_64 -m 2048 -cdrom bin/boot.iso -net none -s -d int

clean:
	@rm -f $(all_obj)
	@rm -f bin/kernel.bin
	@rm -f bin/boot.iso