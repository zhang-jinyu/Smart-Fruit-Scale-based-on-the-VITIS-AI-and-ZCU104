#!/bin/bash

# Update the petalinux makefile to copy the rootfs.tar.gz file to the Vitis platform staging area
sed -i '31i\\tcp -f images\/linux\/rootfs.tar.gz \${OUTPUT}\/src\/\${CPU_ARCH}\/xrt\/image\/rootfs.tar.gz' petalinux/Makefile

# Update the petalinux project for SD card rootfs
cp petalinux/project-spec/configs/config petalinux/project-spec/configs/config.old
cp petalinux/project-spec/configs/rootfs_config petalinux/project-spec/configs/rootfs_config.old
sed -i 's/CONFIG_SUBSYSTEM_ROOTFS_INITRAMFS=y/# CONFIG_SUBSYSTEM_ROOTFS_INITRAMFS is not set/g' petalinux/project-spec/configs/config
sed -i 's/# CONFIG_SUBSYSTEM_ROOTFS_EXT is not set/CONFIG_SUBSYSTEM_ROOTFS_EXT=y/g' petalinux/project-spec/configs/config
echo 'CONFIG_SUBSYSTEM_SDROOT_DEV="/dev/mmcblk0p2"' >> petalinux/project-spec/configs/config

cat <<EOT >> petalinux/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi
/{
  chosen {
    bootargs = "earlycon console=ttyPS0,115200 clk_ignore_unused root=/dev/mmcblk0p2 rw rootwait cma=1100M cpuidle.off=1";
  };
};
EOT

