# -- Bootloader --

bl_cc := clang
bl_cflags := -I efi -target x86_64-pc-win32-coff -fno-stack-protector -fshort-wchar -mno-red-zone
bl_ld := lld-link
bl_lflags := -subsystem:efi_application -nodefaultlib -dll

bl_src := $(shell find bootloader/src/ -iname "*.c")
bl_obj := $(bl_src:bootloader/src/%.c=bootloader/obj/%.o)

all: disk.img

disk.img: bootloader/bootloader.efi
	bash mk_img.sh

bootloader/bootloader.efi: $(bl_obj)
	$(bl_ld) $(bl_lflags) -entry:efi_main $^ -out:$@

bootloader/obj/%.o: bootloader/src/%.c
	-@ mkdir -p $(dir $@)
	$(bl_cc) $(bl_cflags) -c $< -o $@

run: all
	qemu-system-x86_64 -enable-kvm -bios bios.bin disk.img