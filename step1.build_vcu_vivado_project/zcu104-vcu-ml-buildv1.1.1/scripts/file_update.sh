#!/bin/bash

sed -i 's/\/proj\/xbuilds\/2019.2_released\/internal_platforms\/sw\/sysroot\/aarch64-xilinx-linux/$(SYSROOT)/g' $1
sed -i 's/-L\/proj\/xbuilds\/2018.2_released\/installs\/lin64\/SDx\/2018.2\/platforms\/zcu102\/sw\/ocl\/ocl\/image//g' $1
sed -i 's/\/proj\/xresults\/slv\/sivaraj\/sdaccel\/20192\/zcu104_vcu_ml_released/$(PROJ_DIR)\/zcu104_vcu_ml_2019_2_demo/g' $1
sed -i 's/\/proj\/xresults_dcs3\/sivaraj\/sdaccel\/20192\/zcu104_vcu_ml_released/$(PROJ_DIR)\/zcu104_vcu_ml_2019_2_demo/g' $1

