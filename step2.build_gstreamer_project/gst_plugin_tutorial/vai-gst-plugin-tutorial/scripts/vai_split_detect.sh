
### Determine which quad-camera platform being used
###  Note: This script currently only supports ZCU102, ZCU104, and UltraZed-EV
if [[ `hostname` == "zcu102_mc4" ]]; then
  N=24
elif [[ `hostname` == "zcu104_mc4" ]]; then
  N=11
elif [[ `hostname` == "uz7evcc_mc4" ]]; then
  N=7
fi

### Set the number of camera streams to 4
yavta --no-query -w '0x0098c981 4' /dev/v4l-subdev4

### Set the image sensor resolution and format
media-ctl -d /dev/media0 -V "\"AR0231.$N-0011\":0 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"AR0231.$N-0012\":0 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"AR0231.$N-0013\":0 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"AR0231.$N-0014\":0 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"

### Set the SERDES resolution and format
media-ctl -d /dev/media0 -V "\"MAX9286-SERDES.$N-0048\":0 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"MAX9286-SERDES.$N-0048\":1 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"MAX9286-SERDES.$N-0048\":2 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"MAX9286-SERDES.$N-0048\":3 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]"
media-ctl -d /dev/media0 -V "\"MAX9286-SERDES.$N-0048\":4 [fmt:SGRBG8/1920x4320 field:none colorspace:srgb]"

### Set up the CSI Rx subsystem resolution and format
media-ctl -d /dev/media0 -V '"a0060000.csiss":0 [fmt:SGRBG8/1920x4320 field:none]'
media-ctl -d /dev/media0 -V '"a0060000.csiss":1 [fmt:SGRBG8/1920x4320 field:none]'

### Setup the AXI Switch resolution and format
media-ctl -d /dev/media0 -V '"amba:axis_switch@0":0 [fmt:SGRBG8/1920x4320 field:none colorspace:srgb]'
media-ctl -d /dev/media0 -V '"amba:axis_switch@0":1 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]'
media-ctl -d /dev/media0 -V '"amba:axis_switch@0":2 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]'
media-ctl -d /dev/media0 -V '"amba:axis_switch@0":3 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]'
media-ctl -d /dev/media0 -V '"amba:axis_switch@0":4 [fmt:SGRBG8/1920x1080 field:none colorspace:srgb]'

### Set Camera 0 capture pipeline properties, resize from 1920x1080 to 640x360
media-ctl -d /dev/media0 -V '"b0040000.v_demosaic":0 [fmt:SGRBG8/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b0040000.v_demosaic":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b0060000.csc":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b0060000.csc":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b0080000.scaler":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b0080000.scaler":1 [fmt:RBG24/640x360 field:none]'

### Set Camera 1 capture pipeline properties, resize from 1920x1080 to 640x360
media-ctl -d /dev/media0 -V '"b1040000.v_demosaic":0 [fmt:SGRBG8/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b1040000.v_demosaic":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b1060000.csc":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b1060000.csc":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b1080000.scaler":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b1080000.scaler":1 [fmt:RBG24/640x360 field:none]'

### Set Camera 2 capture pipeline properties, resize from 1920x1080 to 640x360
media-ctl -d /dev/media0 -V '"b2040000.v_demosaic":0 [fmt:SGRBG8/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b2040000.v_demosaic":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b2060000.csc":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b2060000.csc":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b2080000.scaler":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b2080000.scaler":1 [fmt:RBG24/640x360 field:none]'

### Set Camera 3 capture pipeline properties, resize from 1920x1080 to 640x360
media-ctl -d /dev/media0 -V '"b3040000.v_demosaic":0 [fmt:SGRBG8/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b3040000.v_demosaic":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b3060000.csc":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b3060000.csc":1 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b3080000.scaler":0 [fmt:RBG24/1920x1080 field:none]'
media-ctl -d /dev/media0 -V '"b3080000.scaler":1 [fmt:RBG24/640x360 field:none]'

### Set Brightness of CSC to 100%
echo 'Setting brightness to 100%'
yavta --no-query -w '0x0098c9a1 100' /dev/v4l-subdev10
yavta --no-query -w '0x0098c9a1 100' /dev/v4l-subdev11
yavta --no-query -w '0x0098c9a1 100' /dev/v4l-subdev12
yavta --no-query -w '0x0098c9a1 100' /dev/v4l-subdev13

gst-launch-1.0 \
    v4l2src device=/dev/video2 io-mode=4 ! \
    video/x-raw, width=640, height=360, format=BGR, framerate=30/1 ! \
    queue ! vaipersondetect ! queue ! \
    fpsdisplaysink video-sink="kmssink bus-id=b00c0000.v_mix plane-id=30 \
    render-rectangle=\"<0,0,640,360>\"" sync=false fullscreen-overlay=true \
    \
    v4l2src device=/dev/video3 io-mode=4 ! \
    video/x-raw, width=640, height=360, format=BGR, framerate=30/1 ! \
    queue ! vaifacedetect ! queue ! \
    fpsdisplaysink video-sink="kmssink bus-id=b00c0000.v_mix plane-id=31 \
    render-rectangle=\"<640,0,640,360>\"" sync=false fullscreen-overlay=true \
    \
    v4l2src device=/dev/video4 io-mode=4 ! \
    video/x-raw, width=640, height=360, format=BGR, framerate=30/1 ! \
    queue ! vaifacedetect ! queue ! \
    fpsdisplaysink video-sink="kmssink bus-id=b00c0000.v_mix plane-id=32 \
    render-rectangle=\"<0,360,640,360>\"" sync=false fullscreen-overlay=true \
    \
    v4l2src device=/dev/video5 io-mode=4 ! \
    video/x-raw, width=640, height=360, format=BGR, framerate=30/1 ! \
    queue ! vaipersondetect ! queue ! \
    fpsdisplaysink video-sink="kmssink bus-id=b00c0000.v_mix plane-id=33 \
    render-rectangle=\"<640,360,640,360>\"" sync=false fullscreen-overlay=true \
    \
    #-v

### Set brightness of CSC back to 50%
yavta --no-query -w '0x0098c9a1 50' /dev/v4l-subdev10
yavta --no-query -w '0x0098c9a1 50' /dev/v4l-subdev11
yavta --no-query -w '0x0098c9a1 50' /dev/v4l-subdev12
yavta --no-query -w '0x0098c9a1 50' /dev/v4l-subdev13

