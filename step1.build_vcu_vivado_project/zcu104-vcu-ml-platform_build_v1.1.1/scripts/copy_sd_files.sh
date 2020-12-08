#!/bin/bash

mkdir -p sd_card
cp -rp $PROJ_DIR/DPU-TRD/prj/Vitis/binary_container_1/sd_card/* sd_card/.
find workspaces/gst* -name "*.so" -exec cp '{}' sd_card/. \;
cp workspaces/xrtutils/Debug/*.so sd_card/.
cp workspaces/rtsp/Debug/rtsp sd_card/.
cp $PROJ_DIR/models/libdpumodel*.so sd_card/.
cp workspaces/sdcard/*.sh sd_card/.
cp -rp workspaces/sdcard/demo_inputs sd_card/.
cp -rp workspaces/sdcard/test_videos sd_card/.

