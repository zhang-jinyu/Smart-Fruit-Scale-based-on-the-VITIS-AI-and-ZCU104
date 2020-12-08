<p>
<div align="center">
  <h1 align="center">TFSSD Gstreamer plugin</h1>
</div>
</p>

# Introduction
This document describes steps that can be used to create a GStreamer Machine Learning plugin that uses the Xilinx Vitis-AI Library. This tutorial provides detailed steps to create face detection and person detection GStreamer plugins.  The plugins are then tested on the <a href="https://xterra2.avnet.com/xilinx/zcu104/zcu104-mc4-ml-example">ZCU104 Quad-Camera + ML Platform</a>.

Since this tutorial uses the ZCU104 Quad-Camera + ML platform you will see references to that directory structure throughout this document.  If you are targeting a different platform then you will need to modify accordingly.  

# Prerequisites

+ Linux host machine
+ <a href="https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c">GStreamer</a>
+ 2019.2 Xilinx tools
+ <a href="https://xterra2.avnet.com/xilinx/zcu104/zcu104-mc4-ml-example">ZCU104 Quad-Camera + ML Platform</a> (or other Xilinx ML development platform with GStreamer installed)

  **NOTE:** If building the ZCU104 Quad-Camera + ML platform from scratch, make sure to include the VART package dependencies under <a href="https://xterra2.avnet.com/xilinx/zcu104/zcu104-mc4-ml-example#part-1-building-the-zcu104-vitis-platform-with-fmc-quad-cam-and-ml-support">Part 1</a> (i.e. follow the instructions on running the ``update_for_vart.sh`` script).

# Overview
This document assumes that the 2019.2 Xilinx tools are set up on the host machine, and a target SDK (sysroot) is available for cross compilation. The structure of this document is organized as follows:

- <a href="#part-0-project-setup">Part 0: Project setup</a>
- <a href="#part-1-setting-up-the-cross-compilation-environment-with-vitis-ai-library-support">Part 1: Setting up the cross-compilation environment with Vitis-AI-Library support</a>
- <a href="#part-2-creating-and-cross-compiling-the-gstreamer-plugin">Part 2: Creating and cross-compiling the GStreamer plugin</a>
- <a href="#part-3-installing-the-vitis-ai-library-on-the-target-hardware">Part 3: Installing the Vitis-AI-Library on the target hardware</a>
- <a href="#part-4-running-on-the-zcu104-quad-camera-ml-target-hardware">Part 4: Running on the ZCU104 Quad-Camera + ML target hardware</a>

Complete source code files can be found in the solution directory of this repository.

# Part 0: Project setup

- Create a project directory of your choosing (i.e. ~/gst_plugin_tutorial) and create an environment variable that points to that location.

  ```bash
  mkdir ~/test/gst_plugin_tutorial
  export GST_PROJ_DIR=~/test/gst_plugin_tutorial
  ```

- Clone this repository
  ```bash
  cd $GST_PROJ_DIR
  git clone https://xterra2.avnet.com/xilinx/ml/vai-gst-plugin-tutorial 
  ```  

# Part 1: Setting up the cross-compilation environment with Vitis-AI-Library support
This section describes how to add Vitis-AI-Library support to an existing PetaLinux SDK.  If your target platform was created by following the <a href="https://xterra2.avnet.com/xilinx/zcu104/zcu104-mc4-ml-example">ZCU104 Quad-Camera + ML Platform</a> instructions then the PetaLinux SDK was created in the ``make peta_sysroot`` step.

**NOTE 1:** If you have already updated your PetaLinux SDK with the Vitis-AI libraries then you can skip this section. This section follows the steps provided in the <a href="https://github.com/Xilinx/Vitis-AI/tree/v1.1/Vitis-AI-Library#setting-up-the-host">Setting Up the Host</a> section of the Vitis-AI-Library setup instructions.  

**NOTE 2:** All steps in this section are performed on the host machine

- Create an environment variable that points to your cross-compilation target root file system. 
  ```bash
  export SYSROOT=~/zcu104_mc4_ml/platform/zcu104_mc4/platform_repo/sysroot/sysroots/aarch64-xilinx-linux
  ``` 

- Source the cross-compilation environment setup script
  ```bash
  unset LD_LIBRARY_PATH
  source $SYSROOT/../../environment-setup-aarch64-xilinx-linux
  ```

- Download the Vitis-AI runtime package
  ```bash
  cd $GST_PROJ_DIR
  wget https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_2019.2-r1.1.0.tar.gz -O vitis_ai_2019.2-r1.1.0.tar.gz
  ```

- Install the Vitis-AI runtime package in the cross-compilation environment
  ```bash
  tar -xvzf vitis_ai_2019.2-r1.1.0.tar.gz -C $SYSROOT
  ```

# Part 2: Creating and cross-compiling the GStreamer plugin
 This section describes how to create a GStreamer video filter plugin from template, and customize it to use the Vitis-AI-Library.  The culmination of this section will result in two custom Vitis-AI plugins - 1) Face Detection and 2) Person Detection.

- Download the GStreamer plugins-bad repository
  ```bash
  cd $GST_PROJ_DIR
  git clone https://github.com/GStreamer/gst-plugins-bad.git
  ```

- Create the face and person detection plugins from the video filter template
  ```bash
  mkdir -p vaitfssd
  
  cd vaitfssd
  ../gst-plugins-bad/tools/gst-element-maker vaitfssd videofilter
  rm *.so *.o
  mv gstvaitfssd.c gstvaitfssd.cpp
  cd ..
  ```

  **NOTE:** You may see a warning indicating ``gst-indent: command not found``.  As long as the plugin ``.c`` and ``.h`` file are created then it should be okay.

- The face and person detection plugins will process video frames in-place instead of copying data from an input buffer to an output buffer.  Remove references in the plugin template code to the ``tranform_frame()`` function, but make sure to leave the references to ``transform_frame_ip()``.  The following commands will delete the lines of code that set the ``transform_frame`` function in the ``class_init()`` function.  
  ```bash
  sed -i '/video_filter_class->transform_frame = /d' vaitfssd/*.cpp
  ```

  The ``transform_frame()`` function is used when data from the input buffer is processed and then copied to a different output buffer.  In this particular application the buffer copy is not necessary and the in-place (``transform_frame_ip``) function is used instead to modify the video frame data.  

- Add the OpenCV and Vitis-AI-Library header files

  * In ``vaitfssd/gstvaitfssd`` add
    ```c++
    /* OpenCV header files */
    #include <opencv2/core.hpp>
    #include <opencv2/opencv.hpp>
    #include <opencv2/highgui.hpp>
    #include <opencv2/imgproc.hpp>

    /* Vitis-AI-Library specific header files */
    #include <vitis/ai/tfssd.hpp>
    #include <vitis/ai/nnpp/tfssd.hpp>
    ```


- Update the pad templates to reflect the supported pixel formats
  * In ``gstvaitfssd.cpp`` Change
    
    **FROM:**
    ```c++
    /* FIXME: add/remove formats you can handle */
    #define VIDEO_SRC_CAPS \
        GST_VIDEO_CAPS_MAKE("{ I420, Y444, Y42B, UYVY, RGBA }")

    /* FIXME: add/remove formats you can handle */
    #define VIDEO_SINK_CAPS \
        GST_VIDEO_CAPS_MAKE("{ I420, Y444, Y42B, UYVY, RGBA }")
    ```  

    **TO:**
    ```c++
    /* Input format */
    #define VIDEO_SRC_CAPS \
        GST_VIDEO_CAPS_MAKE("{ BGR }")

    /* Output format */
    #define VIDEO_SINK_CAPS \
        GST_VIDEO_CAPS_MAKE("{ BGR }")
    ```

- Update the ``*_class_init()`` functions
  * In ``gstvaitfssd.cpp`` change
    
    **FROM:**
    ```c++
    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
        gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS)));
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
        gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS)));

    gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
        "FIXME Long name", "Generic", "FIXME Description",
        "FIXME <fixme@example.com>");
    ```

    **TO:**
    ```c++
    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
        gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS ",width = (int) [1, 640], height = (int) [1, 360]")));
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
        gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS ", width = (int) [1, 640], height = (int) [1, 360]")));

    gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
        "Face detection using the Vitis-AI-Library", 
        "Video Filter", 
        "TFSSD",
        "FIXME <fixme@example.com>");
    ```


- Update the ``transform_frame_ip()`` functions.  The code snippet shown below that is used to draw bounding boxes is based on <a href="https://github.com/Xilinx/Vitis-AI/blob/v1.1/Vitis-AI-Library/ssd/test/test_ssd.cpp">test_ssd.cpp</a> from the Vitis-AI-Library GitHub repository.

  * In ``gstvaitfssd.cpp`` change

    **FROM:**
    ```c++
    static GstFlowReturn
    gst_vaitfssd_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
    {
      GstVaitfssd *vaitfssd = GST_VAITFSSD (filter);

      GST_DEBUG_OBJECT (vaitfssd, "transform_frame_ip");

      return GST_FLOW_OK;
    }
    ``` 

    **TO:**
    ```c++
    static GstFlowReturn
    gst_vaitfssd_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
    {
      GstVaitfssd *vaitfssd = GST_VAITFSSD (filter);

      /* Create ssd detection object */
      thread_local auto ssd = vitis::ai::TFSSD::create("ssd_mobilenet_v1_coco_tf");

      /* Setup an OpenCV Mat with the frame data */
      cv::Mat img(360, 640, CV_8UC3, GST_VIDEO_FRAME_PLANE_DATA(frame, 0));

      /* Perform ssd detection */
      auto results = ssd->run(img);

      /* Draw bounding boxes */
      for (auto &box : results.bboxes)
      {
        int xmin = box.x * img.cols;
        int ymin = box.y * img.rows;
        int xmax = xmin + (box.width * img.cols);
        int ymax = ymin + (box.height * img.rows);

        xmin = std::min(std::max(xmin, 0), img.cols);
        xmax = std::min(std::max(xmax, 0), img.cols);
        ymin = std::min(std::max(ymin, 0), img.rows);
        ymax = std::min(std::max(ymax, 0), img.rows);

        cv::rectangle(img, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 255, 0), 2, 1, 0);
      }

      GST_DEBUG_OBJECT (vaitfssd, "transform_frame_ip");

      return GST_FLOW_OK;
    }
    ```   

  
- Update the plugin package information
  * In ``gstvaitfssd.cpp`` change

    **FROM:**
    ```c++
    #ifndef VERSION
    #define VERSION "0.0.FIXME"
    #endif
    #ifndef PACKAGE
    #define PACKAGE "FIXME_package"
    #endif
    #ifndef PACKAGE_NAME
    #define PACKAGE_NAME "FIXME_package_name"
    #endif
    #ifndef GST_PACKAGE_ORIGIN
    #define GST_PACKAGE_ORIGIN "http://FIXME.org/"
    #endif

    GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        vaitfssd,
        "FIXME plugin description",
        plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
    ``` 

    **TO:**
    ```c++
    #ifndef VERSION
    #define VERSION "0.0.0"
    #endif
    #ifndef PACKAGE
    #define PACKAGE "vaitfssd"
    #endif
    #ifndef PACKAGE_NAME
    #define PACKAGE_NAME "GStreamer Xilinx Vitis-AI-Library"
    #endif
    #ifndef GST_PACKAGE_ORIGIN
    #define GST_PACKAGE_ORIGIN "http://xilinx.com"
    #endif

    GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        vaitfssd,
        "TFSSD using the Xilinx Vitis-AI-Library",
        plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)    
    ```


- Compile the plugins using provided make files
  ```bash
  cd $GST_PROJ_DIR/vaitfssd
  make -f $GST_PROJ_DIR/vai-gst-plugin-tutorial/solution/vaitfssd/Makefile

  ```

- When the compilation completes you should find ``libgstvaitfssd.so``  files  

# Part 3: Installing the Vitis-AI-Library on the target hardware
This section describes how to install the Vitis-AI-Library on the target hardware.  The steps in this section summarize those described on the <a href="https://github.com/Xilinx/Vitis-AI/tree/v1.1/Vitis-AI-Library">Vitis-AI-Library</a> landing page.  
<br>

**NOTE:** The steps in this section should be executed on the target hardware

- Download the Vitis-AI pre-compiled model files
  ```bash
  mkdir -p ~/Downloads
  cd ~/Downloads
  wget https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_model_ZCU102_2019.2-r1.1.0.deb -O vitis_ai_model_ZCU102_2019.2-r1.1.0.deb
  ```

  **NOTE:** The ZCU102 model files are used for both the ZCU104 & ZCU102 Quad-Camera + ML platforms since the DPU was compiled with the RAM_USAGE_LOW option.

- Download the Vitis-AI Runtime
  ```bash
  wget https://www.xilinx.com/bin/public/openDownload?filename=vitis-ai-runtime-1.1.2.tar.gz -O vitis-ai-runtime-1.1.2.tar.gz 
  ```

- Install The Vitis-AI model files
  ```bash
  dpkg -i vitis_ai_model_ZCU102_2019.2-r1.1.0.deb
  ```

- Install the Vitis-AI runtime
  ```bash
  tar -xvzf vitis-ai-runtime-1.1.2.tar.gz 
  cd vitis-ai-runtime-1.1.2
  dpkg -i --force-all unilog/aarch64/libunilog-1.1.0-Linux-build46.deb
  dpkg -i XIR/aarch64/libxir-1.1.0-Linux-build46.deb
  dpkg -i VART/aarch64/libvart-1.1.0-Linux-build48.deb
  dpkg -i Vitis-AI-Library/aarch64/libvitis_ai_library-1.1.0-Linux-build46.deb
  ```

- Modify the ``/etc/vart.conf`` file to point to the correct location for the ``dpu.xclbin``
  ```bash
  sed -i 's/run\/media\/mmcblk0p1/usr\/lib/' /etc/vart.conf
  ```

# Part 4: Running on the ZCU104 Quad-Camera + ML target hardware
This section describes how to run the custom Vitis-AI GStreamer plugins on the ZCU104 Quad-Camera + ML platform from the command line.  


- Copy the compiled ML plugins from the host machine to the target hardware using an Ethernet connection. Exectue the following command on the **host** machine
  ```bash
  cd $GST_PROJ_DIR
  scp vaitfssd/libgstvaitfssd.so root@192.168.4.196:/usr/lib/gstreamer-1.0/.
  scp -r vai-gst-plugin-tutorial/scripts root@$TARGET_IP:~/.
  ```

  **NOTE:** ``$TARGET_IP`` in the command above should be replaced with the IP address of your target hardware
-Testing with the GStreamer face detection plugin has been known to work when setting the DPU clock to 50% with the following command
```
python3 ~/dpu_clk.py 50
```
-Execute the following commands on the ZCU104 to set the display resolution to 480p
```
export DISPLAY=:0.0
xrandr --output DP-1 --mode 640x480
```
- In the previous step a few predefined scripts were copied to the target hardware which set up the image processing input & output pipelines.  Feel free to investigate the scripts.  The command that launches the GStreamer pipeline starts with gst-launch-1.0 and looks like the following:

  ```bash
gst-launch-1.0 -v \
  v4l2src device=/dev/video0 ! \
  video/x-raw, width=640, height=360, format=YUY2, framerate=30/1 ! \
  queue ! \
  videoconvert ! \
  video/x-raw, format=BGR ! \
  queue ! \
  vaitfssd ! \
  queue ! \
  videoconvert ! \
  fpsdisplaysink sync=false text-overlay=false fullscreen-overlay=true
  ```

  **NOTE:** The input and output pipelines need to be setup correctly before launching the GStreamer pipeline.  The above command will not work if the input & output pipelines are not setup.  See the predefined scripts for more details.

  There are three scripts used to execute the GStreamer pipeline
  * ``vai_face_detect.sh``- launches pipeline for 4 cameras with face detection
  * ``vai_person_detect.sh`` - launches pipeline for 4 cameras with person detection
  * ``vai_split_detect.sh`` - launches pipeline for 4 cameras with face detection on 2 feeds, and person detection on the other 2 feeds


- Since both the face and person detection networks expect an input resolution of 640x360 we can set the display resolution to 720p, and display the results in quadrants without having to perform a resolution scaling operation.  Execute the following command on the **target** to change the display resolution to 720p.
  ```bash
  source /media/card/scripts/hdmi_display_720p30.sh
  ```

- To run face detection on the ZCU104 Quad-Camera + ML platform for all 4 cameras execute the following command on the **target**
  ```bash
  ~/scripts/vai_face_detect.sh
  ```

  Press ``ctrl-c`` on your keyboard to stop the GStreamer pipeline

  **NOTE:** If you see an error indicating that the ``vaitfssd`` plugin cannot be found then try deleting the ``~/.cache`` directory with the command
  ```bash
  rm -rf ~/.cache
  ```

- To run person detection on the ZCU104 Quad-Camera + ML platform for all 4 cameras execute the following command on the **target**
  ```bash
  ~/scripts/vai_person_detect.sh
  ```

  Press ``ctrl-c`` on your keyboard to stop the GStreamer pipeline
  
- To run face detection on 2 cameras and person detection on the other 2 cameras execute the following command on the **target**
  ```bash
  ~/scripts/vai_split_detect.sh
  ```

  Press ``ctrl-c`` on your keyboard to stop the GStreamer pipeline


# Summary
This tutorial described detailed steps used to create tfssd GStreamer plugins using the Xilinx Vitis-AI-Library.  The finished plugin source code is included in this repository in the solution directory. 

# References
- <a href="https://github.com/Xilinx/Vitis-AI/tree/master">Vitis-AI</a>
- <a href="https://github.com/Xilinx/Vitis-AI/tree/master/Vitis-AI-Library">Vitis-AI-Library</a>
- <a href="https://xterra2.avnet.com/xilinx/zcu104/zcu104-mc4-ml-example">ZCU104 FMC Quad-Camera + ML Example</a>
- <a href="https://gstreamer.freedesktop.org/">GStreamer</a>
