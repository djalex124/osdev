build_speed:= -O2

kernel_flags := -g -ggdb -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -c
kernel_link  := -g -ggdb -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -T kernel/link.ld

all: run

kernel_src := $(wildcard kernel/*.c)
kernel_gas := $(wildcard kernel/*.S)

kernel_obj_src = $(kernel_src:.c=.o)
kernel_obj_gas = $(kernel_gas:.S=.o)

bin/kernel.bin: $(kernel_obj_src) $(kernel_obj_gas)
	/home/alexk/ccx86_64/bin/x86_64-elf-gcc $(kernel_link) $(kernel_obj_gas) $(kernel_obj_src)  -o bin/kernel.bin -nostdlib -lgcc
	@nm bin/kernel.bin > bin/kernel.map
	@cp bin/kernel.bin bin/dbg_kernel.bin
	@objcopy --strip-debug bin/kernel.bin

%.o: %.c 
	@/home/alexk/ccx86_64/bin/x86_64-elf-gcc $(kernel_flags) -MMD -MP $< -o $@ -lgcc

%.o: %.S 
	@/home/alexk/ccx86_64/bin/x86_64-elf-gcc $(kernel_flags) -MMD -MP $< -o $@ -lgcc

bin/boot.iso: bin/kernel.bin
	@cp bin/kernel.bin grub/boot/kernel.bin
	@grub-mkrescue -o bin/boot.iso grub

run: bin/boot.iso
	@qemu-system-x86_64 -m 2048 -cdrom bin/boot.iso -net none

debug: bin/boot.iso
	@qemu-system-x86_64 -m 2048 -cdrom bin/boot.iso -net none -s -S -d int

clean:
#	@rm $(kernel_obj_src)
	@rm -f $(kernel_obj_gas)
	@rm -f bin/kernel.bin
	@rm -f bin/boot.iso