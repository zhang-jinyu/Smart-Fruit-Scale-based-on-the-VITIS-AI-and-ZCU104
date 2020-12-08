#!/bin/bash

# Build XRT Utils
cd ${PROJ_DIR}/zcu104_vcu_ml_2019_2_demo/workspaces/xrtutils/Debug
make

# Build GST Allocator class
cd ${PROJ_DIR}/zcu104_vcu_ml_2019_2_demo/workspaces/gst/allocators/Debug
make

# Build GST Base class
cd ${PROJ_DIR}/zcu104_vcu_ml_2019_2_demo/workspaces/gst/base/Debug
make

# Build GST Face Detection plug-in
cd ${PROJ_DIR}/zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxfacedetect/Debug
make

# Build GST Traffic Detection plug-in
cd ${PROJ_DIR}/zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxtrafficdetect/Debug
make

# Build RTSP source
cd ${PROJ_DIR}/zcu104_vcu_ml_2019_2_demo/workspaces/rtsp/Debug
make

