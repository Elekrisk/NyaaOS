#! /usr/bin/env bash

set -e

losetup -a | grep "$(pwd)/disk.img" | cut -d':' -f1 | xargs -r sudo losetup -d

dd if=/dev/zero of=disk.img bs=1M count=512
parted disk.img mklabel gpt
sudo losetup -f disk.img
LOOPBACK=$(losetup -a | grep "$(pwd)/disk.img" | cut -d':' -f1 | head -n1)

# shellcheck disable=SC2024
sudo sfdisk "$LOOPBACK" < disk.img.sfdisk
sudo partprobe "$LOOPBACK"

sudo mkfs.fat -F 32 "${LOOPBACK}p1"

mkdir mnt
sudo mount "${LOOPBACK}p1" mnt
sudo mkdir -p mnt/efi/boot
sudo cp bootloader/bootloader.efi mnt/efi/boot/bootx64.efi

sudo umount mnt
rmdir mnt

losetup -a | grep "$(pwd)/disk.img" | cut -d':' -f1 | xargs sudo losetup -d
