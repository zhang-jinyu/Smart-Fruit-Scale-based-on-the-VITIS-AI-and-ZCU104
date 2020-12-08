gst-launch-1.0 \
  v4l2src device=/dev/video0 ! \
  video/x-raw, width=640, height=360, format=YUY2, framerate=30/1 ! \
  queue ! \
  videoconvert ! \
  video/x-raw, format=BGR ! \
  queue ! \
  vaifacedetect ! \
  vaipersondetect ! \
  queue ! \
  videoscale ! \
  video/x-raw, width=640, height=480 ! \
  fpsdisplaysink video-sink="kmssink bus-id=fd4a0000.zynqmp-display plane-id=38" sync=false fullscreen-overlay=true

