
### Set the display resolution to 480p
export DISPLAY=:0.0
xrandr --output DP-1 --mode 640x480
xset -dpms
modetest -D fd4a0000.zynqmp-display -s 42@40:640x480-60@AR24 &
sleep 1
modetest -D fd4a0000.zynqmp-display -w 39:alpha:0
