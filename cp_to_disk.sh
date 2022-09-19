#!/usr/bin/bash

set -e

DEVICE=/dev/sda3

mkdir -p mnt
sudo mount /dev/sda3 mnt

sudo mkdir -p mnt/efi/boot
sudo cp bootloader/bootloader.efi mnt/efi/boot/bootx64.efi
sudo cp kernel/kernel.elf mnt/kernel.elf

sudo umount mnt
rmdir mnt
