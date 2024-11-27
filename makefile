build_speed = -O2

gcc = x86_64-elf-gcc

kernel_build = $$(cat build.txt)

kernel_flags = -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -DAQUA_VER_BUILD=$(kernel_build) -gdwarf-5 -fno-pie -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall
kernel_link  = -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -gdwarf-5 -fno-pie -T bin/link.ld

all: run

src_c := $(shell find src/ -name '*.c')
src_s := $(shell find src/ -name '*.S')
obj := $(src_c:.c=.o) $(src_s:.S=.o)

bin/link.ld:
	@$(gcc) -E -P -x c $(kernel_flags) src/kernel/link.ld >bin/link.ld

bin/kernel.bin: $(obj) bin/link.ld
	@$(gcc) $(kernel_link) $(obj) -o bin/kernel.bin -nostdlib -lgcc
	@objcopy --strip-debug bin/kernel.bin

bin/dbg_kernel.bin: $(obj) bin/link.ld
	@$(gcc) $(kernel_link) $(obj) -o bin/dbg_kernel.bin -nostdlib -lgcc

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
	@qemu-system-x86_64 -machine q35 -m 2048 -cdrom bin/boot.iso -net none

debug: kernel_flags += -DAQUA_DEBUG
debug: bin/dbg_boot.iso
	@qemu-system-x86_64 -machine q35 -m 2048 -cdrom bin/boot.iso -net none -s

clean:
	@rm -f bin/link.ld
	@rm -f $(obj)
	@rm -f bin/kernel.bin bin/dbg_kernel.bin
	@rm -f bin/boot.iso