build_speed = -O2

gcc = /usr/opt/cross/compiler/bin/x86_64-elf-gcc

kernel_build = $$(cat build.txt)

kernel_flags = -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -DAQUA_VER_BUILD=$(kernel_build) -ggdb -fno-pie -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall
kernel_link  = -ffreestanding -Iinc -fno-omit-frame-pointer $(build_speed) -ggdb -fno-pie -T bin/link.ld

all: run

src_c := $(shell find src/kernel/ -name '*.c')
src_s := $(shell find src/kernel/ -name '*.S')
obj := $(subst src, obj, $(src_c:.c=.o) $(src_s:.S=.o))

deps := $(subst .o, .d, $(obj))

bin/link.ld:
	@$(gcc) -E -P -x c $(kernel_flags) src/kernel/link.ld >bin/link.ld

drive/kernel.bin: $(obj) bin/link.ld
	@echo $$(($$(cat build.txt) + 1)) > build.txt
	@$(gcc) $(kernel_link) $(obj) -o drive/kernel.bin -nostdlib -lgcc
	@objcopy --only-keep-debug drive/kernel.bin bin/kernel.map
	@objcopy --strip-debug drive/kernel.bin

drive/dbg_kernel.bin: $(obj) bin/link.ld
	@echo $$(($$(cat build.txt) + 1)) > build.txt
	@$(gcc) $(kernel_link) $(obj) -o drive/kernel.bin -nostdlib -lgcc
	@objcopy --only-keep-debug drive/kernel.bin bin/kernel.map

obj/%.o: src/%.c 
	@mkdir -p $(@D)
	@$(gcc) $(kernel_flags) -c -MMD -MP $< -o $@ -lgcc

obj/%.o: src/%.S 
	@mkdir -p $(@D)
	@$(gcc) $(kernel_flags) -c -DASSEMBLY -MMD -MP $< -o $@ -lgcc

efi_cc := /usr/bin/gcc

gnu_efi_inc := /usr/include/efi
gnu_efi_lib := /usr/lib64

drive/boot.efi:
	gcc -Iinc -I$(gnu_efi_inc) -O2 -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/boot/uefiboot.c -o src/boot/uefiboot.o
	ld -shared -Bsymbolic -L$(gnu_efi_lib) -T$(gnu_efi_lib)/elf_x86_64_efi.lds $(gnu_efi_lib)/crt0-efi-x86_64.o src/boot/uefiboot.o -o src/boot/boot.so -lgnuefi -lefi
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 src/boot/boot.so drive/boot.efi

run: drive/boot.efi drive/kernel.bin
	@qemu-system-x86_64 -drive if=pflash,format=raw,unit=0,file=firmware/OVMF_CODE.fd,readonly=on \
					    -drive if=pflash,format=raw,unit=1,file=firmware/OVMF_VARS.fd \
					    -drive file=fat:rw:drive/,format=raw,media=disk -m 2048 -smp 2

debug: kernel_flags += -DAQUA_DEBUG
debug: drive/boot.efi drive/dbg_kernel.bin
	@qemu-system-x86_64 -drive if=pflash,format=raw,unit=0,file=firmware/OVMF_CODE.fd,readonly=on \
					    -drive if=pflash,format=raw,unit=1,file=firmware/OVMF_VARS.fd \
					    -drive file=fat:rw:drive/,format=raw,media=disk -m 2048 -s -S -serial stdio -smp 2

clean:
	@rm -f bin/link.ld
	@rm -r obj/
	@rm -f drive/boot.efi
	@rm -f drive/kernel.bin drive/dbg_kernel.bin