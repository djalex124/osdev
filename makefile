include config.mk

build_speed = -O2

kernel_build = $$(cat build.txt)

kernel_flags = -ffreestanding -I$(kernel_headers) -Iinc -fno-omit-frame-pointer $(build_speed) -DAQUA_VER_BUILD=$(kernel_build) -gdwarf -fno-pie -mcmodel=large -mno-red-zone -Wall
kernel_link  = -ffreestanding -I$(kernel_headers) -Iinc -fno-omit-frame-pointer $(build_speed) -gdwarf -fno-pie -T bin/link.ld

debug_flag =

all: run

src_c := $(shell find src/kernel/ -name '*.c')
src_s := $(shell find src/kernel/ -name '*.S')
obj := $(subst src, obj, $(src_c:.c=.o) $(src_s:.S=.o))

deps := $(subst .o, .d, $(obj))

bin/link.ld:
	@$(gcc) -E -P -x c $(debug_flag) $(kernel_flags) src/kernel/link.ld >bin/link.ld

drive/kernel.bin: $(obj) bin/link.ld
	@echo $$(($$(cat build.txt) + 1)) > build.txt
	@$(gcc) $(kernel_link) $(obj) -o drive/kernel.bin -nostdlib -lgcc
	@$(objcopy) --only-keep-debug drive/kernel.bin bin/kernel.map
	@$(objcopy) --strip-debug drive/kernel.bin

drive/dbg_kernel.bin: $(obj) bin/link.ld
	@echo $$(($$(cat build.txt) + 1)) > build.txt
	@$(gcc) $(kernel_link) $(obj) -o drive/kernel.bin -nostdlib -lgcc
	@$(objcopy) --only-keep-debug drive/kernel.bin bin/kernel.map
	@$(objcopy) --strip-debug drive/kernel.bin
	@cp bin/kernel.map drive/kernel.map

obj/kernel/kernel/kstring.o: src/kernel/kernel/kstring.c
	@mkdir -p $(@D)
	@$(gcc) $(debug_flag) $(kernel_flags) -c -MMD -MP $< -o $@ -lgcc

obj/kernel/%.o: src/kernel/%.c 
	@mkdir -p $(@D)
	@$(gcc) $(debug_flag) $(kernel_flags) -mno-sse -c -MMD -MP $< -o $@ -lgcc

obj/kernel/%.o: src/kernel/%.S 
	@mkdir -p $(@D)
	@$(gcc) $(debug_flag) $(kernel_flags) -c -DASSEMBLY -MMD -MP $< -o $@ -lgcc

obj/boot/uefiboot.o: src/boot/uefiboot.c
	@mkdir -p obj/boot
	@$(efi_cc) $(debug_flag) -DAQUA_VER_BUILD=$(kernel_build) -MMD -MP -Iinc -I$(gnu_efi_inc) $(build_speed) -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/boot/uefiboot.c -o obj/boot/uefiboot.o

drive/EFI/BOOT/BOOTX64.EFI: obj/boot/uefiboot.o
	@$(efi_ld) -shared -Bsymbolic -L$(gnu_efi) -T$(gnu_efi)/elf_x86_64_efi.lds $(gnu_efi)/crt0-efi-x86_64.o obj/boot/uefiboot.o -o obj/boot/boot.so -lgnuefi -lefi
	@$(objcopy) -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --output-target efi-app-x86_64 --subsystem=10 obj/boot/boot.so drive/boot.efi
	@mkdir -p drive/EFI/BOOT
	@mv drive/boot.efi drive/EFI/BOOT/BOOTX64.EFI

drive/test.bin: src/programs/test.c
	@$(gcc) -ffreestanding -fno-pie -mcmodel=large -mno-red-zone -Wall -c $< -o $@

build_run: drive/EFI/BOOT/BOOTX64.EFI drive/kernel.bin drive/test.bin

run: build_run
	@qemu-system-x86_64 -drive if=pflash,format=raw,unit=0,file=firmware/OVMF_CODE.fd,readonly=on \
					    -drive if=pflash,format=raw,unit=1,file=firmware/OVMF_VARS.fd \
					    -drive file=fat:rw:drive/,format=raw,media=disk -m 512 -smp 2

build_debug: debug_flag += -DAQUA_DEBUG
build_debug: drive/EFI/BOOT/BOOTX64.EFI drive/dbg_kernel.bin drive/test.bin

debug: build_debug
	@qemu-system-x86_64 -drive if=pflash,format=raw,unit=0,file=firmware/OVMF_CODE.fd,readonly=on \
					    -drive if=pflash,format=raw,unit=1,file=firmware/OVMF_VARS.fd \
					    -drive file=fat:rw:drive/,format=raw,media=disk -m 512 -s -serial stdio -smp 2

image_run: drive/EFI/BOOT/BOOTX64.EFI drive/kernel.bin drive/test.bin
	@dd if=/dev/zero of=bin/dev.img count=10 bs=1M
	@mkfs.vfat bin/dev.img

	@mcopy -i bin/dev.img drive/kernel.bin ::/
	@mcopy -i bin/dev.img drive/startup.nsh ::/
	@mcopy -i bin/dev.img drive/image.tga ::/
	@mcopy -i bin/dev.img drive/test.bin ::/
	@mmd -i bin/dev.img ::EFI
	@mmd -i bin/dev.img ::EFI/BOOT
	@mcopy -i bin/dev.img drive/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI

	@qemu-img convert -f raw -O qcow2 bin/dev.img bin/dev.qcow2
	@rm bin/dev.img

image_debug: debug_flag += -DAQUA_DEBUG
image_debug: drive/EFI/BOOT/BOOTX64.EFI drive/dbg_kernel.bin drive/test.bin
	@dd if=/dev/zero of=bin/dev.img count=10 bs=1M
	@mkfs.vfat bin/dev.img

	@mcopy -i bin/dev.img drive/kernel.bin ::/
	@mcopy -i bin/dev.img drive/kernel.map ::/
	@mcopy -i bin/dev.img drive/startup.nsh ::/
	@mcopy -i bin/dev.img drive/image.tga ::/
	@mcopy -i bin/dev.img drive/test.bin ::/
	@mmd -i bin/dev.img ::EFI
	@mmd -i bin/dev.img ::EFI/BOOT
	@mcopy -i bin/dev.img drive/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI

	@qemu-img convert -f raw -O qcow2 bin/dev.img bin/dev.qcow2
	@rm bin/dev.img

clean:
	@rm -f bin/link.ld bin/kernel.map
	@rm -r obj/
	@rm -f drive/EFI/BOOT/BOOTX64.EFI
	@rm -f drive/kernel.bin drive/dbg_kernel.bin drive/kernel.map drive/test.bin
	@rm -f bin/dev.qcow2