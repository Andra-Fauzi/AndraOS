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