#!/bin/bash
set -e

DISK=disk.img
SIZE_MB=32

echo "[1] Membuat disk image ${SIZE_MB}MB"
dd if=/dev/zero of=$DISK bs=1M count=$SIZE_MB

echo "[2] Membuat partition table (MBR)"
parted $DISK --script mklabel msdos

echo "[3] Membuat partisi FAT16 (1MiB - end)"
parted $DISK --script mkpart primary fat16 1MiB 100%

echo "[4] Attach ke loop device"
LOOP=$(sudo losetup --find --show --partscan $DISK)
echo "Loop device: $LOOP"

PART=${LOOP}p1

echo "[5] Format FAT16 pada $PART"
sudo mkfs.fat -F 16 $PART

echo "[6] Mount partisi"
sudo mkdir -p /mnt/disk
sudo mount $PART /mnt/disk

echo "[7] Membuat folder GRUB"
sudo mkdir -p /mnt/disk/boot/grub

echo "[8] Copy kernel (kernel.elf) ke disk"
sudo cp kernel.elf /mnt/disk/boot/

echo "[9] Buat grub.cfg"
sudo bash -c 'cat > /mnt/disk/boot/grub/grub.cfg <<EOF
set timeout=0
set default=0

menuentry \"AndraOS\" {
    multiboot /boot/kernel.elf
    boot
}
EOF'

echo "[10] Install GRUB i386-pc"
sudo grub-install \
    --target=i386-pc \
    --boot-directory=/mnt/disk/boot \
    $LOOP

echo "[11] Unmount dan detach"
sudo umount /mnt/disk
sudo losetup -d $LOOP

echo "Selesai! Jalankan dengan:"
echo "qemu-system-i386 -drive format=raw,file=$DISK"
