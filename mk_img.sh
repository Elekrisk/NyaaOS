#! /usr/bin/env bash

set -e

if [ "${1:-nope}" = "debug" ]
then
    DISK=disk-debug.img
    EFI=bootloader/bootloader-debug.efi
# shellcheck disable=SC2037
    SUDO="sudo -S"
else
    DISK=disk.img
    EFI=bootloader/bootloader.efi
    SUDO=sudo
fi

losetup -a | grep "$(pwd)/$DISK" | cut -d':' -f1 | xargs -r $SUDO losetup -d

dd if=/dev/zero of=$DISK bs=1M count=512
parted $DISK mklabel gpt
$SUDO losetup -f $DISK
LOOPBACK=$(losetup -a | grep "$(pwd)/$DISK" | cut -d':' -f1 | head -n1)

# shellcheck disable=SC2024
$SUDO sfdisk "$LOOPBACK" < disk.img.sfdisk
$SUDO partprobe "$LOOPBACK"

$SUDO mkfs.fat -F 32 "${LOOPBACK}p1"

$SUDO mkdir -p mnt
$SUDO mount "${LOOPBACK}p1" mnt
$SUDO mkdir -p mnt/efi/boot
$SUDO cp "$EFI" mnt/efi/boot/bootx64.efi
$SUDO cp kernel/kernel.elf mnt/kernel.elf

$SUDO umount mnt
rmdir mnt

losetup -a | grep "$(pwd)/$DISK" | cut -d':' -f1 | xargs $SUDO losetup -d
