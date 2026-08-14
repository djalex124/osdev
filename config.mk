gcc := x86_64-elf-gcc
ld := x86_64-elf-ld
objcopy := objcopy

kernel_headers := /usr/include

efi_cc := gcc
efi_ld := ld

gnu_efi_inc := /usr/include/efi -I/usr/include/efi/x86_64
gnu_efi := /usr/lib64