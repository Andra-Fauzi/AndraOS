#!/bin/bash
set -e

DISK=disk.img

echo "[1] Attach ke loop device"
LOOP=$(sudo losetup --find --show --partscan $DISK)
echo "Loop device: $LOOP"

PART=${LOOP}p1

echo "[2] Mount partisi"
sudo mkdir -p /mnt/disk
sudo mount $PART /mnt/disk

echo "[3] Copy kernel (kernel.elf) ke disk"
sudo cp kernel.elf /mnt/disk/boot/

echo "[4] Unmount dan detach"
sudo umount /mnt/disk
sudo losetup -d $LOOP

echo "Selesai!"
