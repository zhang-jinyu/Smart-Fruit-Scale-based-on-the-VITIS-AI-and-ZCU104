#!/bin/bash

###
### This script updates the petalinux project to add libraries/packages needed for the Vitis-AI Runtime (VART)
###

# Download recipes from the zcu104_dpu platform repository
wget https://raw.githubusercontent.com/Xilinx/Vitis_Embedded_Platform_Source/2019.2/Xilinx_Official_Platforms/zcu104_dpu/petalinux/project-spec/meta-user/recipes-core/packagegroups/nativesdk-packagegroup-sdk-host.bbappend -O petalinux/project-spec/meta-user/recipes-core/packagegroups/nativesdk-packagegroup-sdk-host.bbappend 
wget https://raw.githubusercontent.com/Xilinx/Vitis_Embedded_Platform_Source/2019.2/Xilinx_Official_Platforms/zcu104_dpu/petalinux/project-spec/meta-user/recipes-core/packagegroups/packagegroup-petalinux-xlnx-ai.bb -O petalinux/project-spec/meta-user/recipes-core/packagegroups/packagegroup-petalinux-xlnx-ai.bb

# Modify the Xilinx-AI package group to remove the Weston/Wayland desktop environment
AI_FILE=petalinux/project-spec/meta-user/recipes-core/packagegroups/packagegroup-petalinux-xlnx-ai.bb
sed -i '/auto-resize/d' $AI_FILE
sed -i '/ai-camera/d' $AI_FILE
sed -i '/base-files/d' $AI_FILE
sed -i '/dpuclk/d' $AI_FILE
sed -i '/screen-flicker/d' $AI_FILE
sed -i '/packagegroup-petalinux-weston/d' $AI_FILE
sed -i '/xfce4-terminal/d' $AI_FILE

# Update the PetaLinux project for Xilinx Vitis AI library support
echo 'CONFIG_packagegroup-petalinux-xlnx-ai=y' >> petalinux/project-spec/configs/rootfs_config
echo 'CONFIG_packagegroup-petalinux-self-hosted=y' >> petalinux/project-spec/configs/rootfs_config
echo 'CONFIG_imagefeature-package-management=y' >> petalinux/project-spec/configs/rootfs_config
echo 'CONFIG_imagefeature-debug-tweaks=y' >> petalinux/project-spec/configs/rootfs_config
echo 'PACKAGE_CLASSES = " package_deb"' >> petalinux/project-spec/meta-user/conf/petalinuxbsp.conf
echo 'CONFIG_packagegroup-petalinux-xlnx-ai' >> petalinux/project-spec/meta-user/conf/user-rootfsconfig

