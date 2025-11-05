build_uname = $(shell uname -m)

build_speed = -O2

gcc = x86_64-elf-gcc
objcopy = x86_64-elf-objcopy

kernel_build = $$(cat build.txt)

ifeq ($(build_uname), x86_64)
kernel_headers = /usr/include
else
kernel_headers = /home/alex/opt/cross/x86_64-elf/include
endif

kernel_flags = -ffreestanding -I$(kernel_headers) -Iinc -fno-omit-frame-pointer $(build_speed) -DAQUA_VER_BUILD=$(kernel_build) -gdwarf -fno-pie -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall
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

obj/%.o: src/%.c 
	@mkdir -p $(@D)
	@$(gcc) $(debug_flag) $(kernel_flags) -c -MMD -MP $< -o $@ -lgcc

obj/%.o: src/%.S 
	@mkdir -p $(@D)
	@$(gcc) $(debug_flag) $(kernel_flags) -c -DASSEMBLY -MMD -MP $< -o $@ -lgcc

efi_cc := x86_64-linux-gnu-gcc

ifeq ($(build_uname), x86_64)
gnu_efi_inc := /usr/include/efi
gnu_efi_lib := /usr/lib
else
gnu_efi_inc := /usr/local/include/efi
gnu_efi_lib := /usr/local/lib
endif

drive/EFI/BOOT/BOOTX64.EFI:
	@x86_64-linux-gnu-gcc $(debug_flag) -Iinc -I$(gnu_efi_inc) $(build_speed) -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/boot/uefiboot.c -o src/boot/uefiboot.o
	@x86_64-linux-gnu-ld -shared -Bsymbolic -L$(gnu_efi_lib) -T$(gnu_efi_lib)/elf_x86_64_efi.lds $(gnu_efi_lib)/crt0-efi-x86_64.o src/boot/uefiboot.o -o src/boot/boot.so -lgnuefi -lefi
	@x86_64-linux-gnu-objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 src/boot/boot.so drive/boot.efi
	@mkdir -p drive/EFI/BOOT
	@mv drive/boot.efi drive/EFI/BOOT/BOOTX64.EFI

# super user is required for mouse movement due to current QEMU quirkiness
# - first movement packet is sent, but none after
# - currently works natively on QEMU for windows and under su for linux

build_run: drive/EFI/BOOT/BOOTX64.EFI drive/kernel.bin

run: build_run
	@sudo qemu-system-x86_64 -drive if=pflash,format=raw,unit=0,file=firmware/OVMF_CODE.fd,readonly=on \
					    -drive if=pflash,format=raw,unit=1,file=firmware/OVMF_VARS.fd \
					    -drive file=fat:rw:drive/,format=raw,media=disk -m 512 -smp 2 -machine q35

build_debug: debug_flag += -DAQUA_DEBUG
build_debug: drive/EFI/BOOT/BOOTX64.EFI drive/dbg_kernel.bin

debug: build_debug
	@sudo qemu-system-x86_64 -drive if=pflash,format=raw,unit=0,file=firmware/OVMF_CODE.fd,readonly=on \
					    -drive if=pflash,format=raw,unit=1,file=firmware/OVMF_VARS.fd \
					    -drive file=fat:rw:drive/,format=raw,media=disk -m 512 -s -serial stdio -smp 2 -machine q35

image_run: drive/EFI/BOOT/BOOTX64.EFI drive/kernel.bin
	@dd if=/dev/zero of=bin/dev.img count=10 bs=1M
	@mkfs.vfat -F 16 bin/dev.img

	@mmd -i bin/dev.img ::EFI
	@mmd -i bin/dev.img ::EFI/BOOT
	@mcopy -i bin/dev.img drive/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i bin/dev.img drive/kernel.bin ::/
	@mcopy -i bin/dev.img drive/startup.nsh ::/

	@qemu-img convert -f raw -O qcow2 bin/dev.img bin/dev.qcow2
	@rm bin/dev.img

image_debug: debug_flag += -DAQUA_DEBUG
image_debug: drive/EFI/BOOT/BOOTX64.EFI drive/dbg_kernel.bin
	@dd if=/dev/zero of=bin/dev.img count=10 bs=1M
	@mkfs.vfat -F 16 bin/dev.img

	@mmd -i bin/dev.img ::EFI
	@mmd -i bin/dev.img ::EFI/BOOT
	@mcopy -i bin/dev.img drive/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i bin/dev.img drive/kernel.bin ::/
	@mcopy -i bin/dev.img drive/kernel.map ::/
	@mcopy -i bin/dev.img drive/startup.nsh ::/

	@qemu-img convert -f raw -O qcow2 bin/dev.img bin/dev.qcow2
	@rm bin/dev.img

clean:
	@rm -f bin/link.ld bin/kernel.map
	@rm -f boot/boot.so boot/uefiboot.o
	@rm -r obj/
	@rm -f drive/EFI/BOOT/BOOTX64.EFI
	@rm -f drive/kernel.bin drive/dbg_kernel.bin drive/kernel.map
	@rm -f bin/dev.qcow2