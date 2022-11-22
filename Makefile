# -- Bootloader --

SHELL := bash

bl_cc := clang
bl_cflags := -masm=intel -O0 -g -I efi -target x86_64-pc-win32-coff -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -Werror -Wno-error=unused-variable -Wno-error=incompatible-library-redeclaration -Wno-error=macro-redefined
bl_ld := lld-link
bl_lflags := -debug:full -subsystem:efi_application -nodefaultlib -dll

bl_src := $(shell find bootloader/src/ -iname "*.c")
bl_src_no_main := $(shell find bootloader/src/ -iname "*.c" -not -iname "main.c")
bl_obj := $(bl_src:bootloader/src/%.c=bootloader/obj/%.o)
bl_obj_no_main := $(bl_src_no_main:bootloader/src/%.c=bootloader/obj/%.o)

k_cc := clang
k_cflags := -ffreestanding
k_ld := ld.lld
k_lflags := -nostdlib -T kernel/link.ld

k_src := $(shell find kernel/src/ -iname "*.c")
k_obj :=$(k_src:kernel/src/%.c=kernel/obj/%.o)

all: disk.img

debug: disk-debug.img

disk-debug.img: bootloader/bootloader-debug.efi kernel/kernel.elf
	./mk_img.sh debug

disk.img: bootloader/bootloader.efi kernel/kernel.elf
	./mk_img.sh

bootloader/bootloader-debug.efi: bootloader/obj/main-debug.o $(bl_obj_no_main)
	$(bl_ld) $(bl_lflags) -entry:efi_main $^ -out:$@

bootloader/obj/main-debug.o: bootloader/src/main.c
	-@ mkdir -p $(dir $@)
	$(bl_cc) $(bl_cflags) -DSENDLOADADDR -c $< -o $@

bootloader/bootloader.efi: $(bl_obj)
	$(bl_ld) $(bl_lflags) -entry:efi_main $^ -out:$@

bootloader/obj/%.o: bootloader/src/%.c
	-@ mkdir -p $(dir $@)
	$(bl_cc) $(bl_cflags) -c $< -o $@

kernel/kernel.elf: $(k_obj)
	$(k_ld) $(k_lflags) $^ -o $@

kernel/obj/%.o: kernel/src/%.c
	-@ mkdir -p $(dir $@)
	$(k_cc) $(k_cflags) -c $< -o $@

run: all
	qemu-system-x86_64 -enable-kvm -bios bios.bin disk.img 

run-debug: debug
	qemu-system-x86_64 -bios bios.bin disk-debug.img -s -S -serial tcp:localhost:12345,server

copy-to-disk: bootloader/bootloader.efi kernel/kernel.elf
	bash cp_to_disk.sh


clean:
	-rm disk.img
	-rm disk-debug.img
	-rm -r bootloader/obj
	-rm -r bootloader/bootloader.*
	-rm -r bootloader/bootloader-debug.*
	-rm -r kernel/obj
	-rm -r kernel/kernel.elf