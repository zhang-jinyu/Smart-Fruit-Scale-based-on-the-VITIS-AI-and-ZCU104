<p>
<div align="center">
  <h1 align="center">ZCU104 VCU + ML Platform & Demo Build Example</h1>
</div>
</p>

# Introduction
This document describes steps that can be used to build the ``ZCU104 VCU 8-channel video decode + ML`` demo provided on the Xilinx download page - https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-platforms/2019-2.html

# Prerequisites

+ Linux host machine
+ 2019.2 Xilinx tools
+ Docker
+ HDMI IP License (if building or modifying the platform)

# Introduction
This document assumes that the 2019.2 Xilinx tools are set up on the host machine.  The structure of this document is organized as follows:

- <a href="#part-0-project-set-up">Part 0: Project set up</a>
- <a href="#part-1-building-the-zcu104-vitis-platform-with-vcu-and-ml-support">Part 1: Building the ZCU104 Vitis platform with VCU and ML support</a>
- <a href="#part-2-adding-the-dpu-to-the-platform">Part 2: Adding the DPU to the platform</a>
- <a href="#part-3-compiling-the-demo-software">Part 3: Compiling the demo software</a>
- <a href="#part-4-running-on-the-zcu104-target-hardware">Part 4: Running on the ZCU104 target hardware</a>

# Part 0: Project set up

- Create a project directory of your choosing (i.e. ~/zcu104_vcu_ml) and create an environment variable that points to that location.

  ```bash
  mkdir ~/zcu104_vcu_ml
  export PROJ_DIR=~/zcu104_vcu_ml
  ```

- Clone this repository
  ```bash
  cd $PROJ_DIR
  git clone https://xterra2.avnet.com/xilinx/ZCU104/zcu104-vcu-ml-build-example
  ```

# Part 1: Building the ZCU104 Vitis platform with VCU and ML support
This section describes how to build the Vitis embedded platform with VCU and ML support.  

- From your Linux host machine clone the Xilinx platform repository
  ```bash
  cd $PROJ_DIR
  git clone https://github.com/Xilinx/Vitis_Embedded_Platform_Source.git
  ```

- Navigate to the zcu104_vcu_ml directory in the cloned repository
  ```bash
  cd Vitis_Embedded_Platform_Source/Xilinx_Official_Platforms/zcu104_vcu_ml
  ```

- If you are using Ubuntu 18.04 and have less than 16 cores on your machine then you will need to modify the Vivado build script to reduce the number of parallel jobs.  The path to the script is ``vivado/zcu104_vcu_ml_xsa.tcl``.  Searching for the keyword ``launch_runs`` will show that the default script is setup to run 16 jobs.  On my virtual machine this causes the build to fail since my machine only has 6 cores.  

  ```bash
  cat -n vivado/zcu104_vcu_ml_xsa.tcl | grep launch_runs
    2734  launch_runs impl_1 -to_step write_bitstream -jobs 16
  ```

  You may need to reduce the number of jobs to a suitable number for your machine

- The ``zcu104_vcu_ml`` platform located in the Xilinx platform repository needs to be modified to host the root file system on the SD card instead of in RAM.  Execute the following command to update petalinux project:
  ```bash
  bash -x $PROJ_DIR/zcu104-vcu-ml-build-example/scripts/update_plnx.sh
  ```

- If you would like to use the Vitis-AI Runtime and Vitis-AI Library then you will need to add additional packages.  If you only plan to use the legacy DNNDK APIs then you do not need to execute this step.
  ```bash
  bash -x $PROJ_DIR/zcu104-vcu-ml-build-example/scripts/update_for_vart.sh
  ```

- Build the platform (this will take some time)
  ```bash
  make all
  ```

- Generate the root file system for cross-compilation
  
  ```bash
  make peta_sysroot
  ```

- Create an environment variable that points to the platform location
  ```bash
  export PFM_DIR=$PROJ_DIR/Vitis_Embedded_Platform_Source/Xilinx_Official_Platforms/zcu104_vcu_ml/platform_repo/zcu104_vcu_ml/export/zcu104_vcu_ml
  ```

# Part 2: Adding the DPU to the platform
The pre-built zcu104_vcu_ml <a href="https://www.xilinx.com/member/forms/download/design-license-zcu104-vcu-8channel.html?filename=zcu104_vcu_ml_2019_2_platform.zip">platform</a> and <a href="https://www.xilinx.com/member/forms/download/design-license-zcu104-vcu-8channel.html?filename=zcu104_vcu_ml_2019_2_demo.zip">demo package</a> are set up for Vitis-AI v1.0.  This section will use the DPU-TRD from the Vitis-AI v1.1 release to update to the latest version of the DPU IP.  This will force us to recompile the face detection and traffic detection models from the Xilinx Model Zoo.  The instructions for recompiling the models is provided in <a href="#part-3-compiling-the-demo-software">Part 3</a>.

- Clone the Vitis-AI v1.1 repository
  ```bash
  git clone --branch v1.1 --single-branch https://github.com/Xilinx/Vitis-AI ~/Vitis-AI-v1.1
  ```

- Copy the DPU-TRD directory from the Vitis-AI v1.1 repository to the project directory
  ```bash
  cp ~/Vitis-AI-v1.1/DPU-TRD $PROJ_DIR/.
  ```

- Modify the DPU configuration to enable URAM and the project configuration for 1 DPU kernel. 
  ```bash
  cd $PROJ_DIR/DPU-TRD/prj/Vitis
  bash -x update_dpu_config.sh
  ```

- Source the XRT setup script (your path may vary)
  ```bash
  source /opt/xilinx/xrt/setup.sh
  ```

- Build with default DPU configuration
  ```bash
  cd $PROJ_DIR/DPU-TRD/prj/Vitis
  export SDX_PLATFORM=$PFM_DIR/zcu104_vcu_ml.xpfm
  make KERNEL=DPU DEVICE=zcu104
  ```

# Part 3: Compiling the demo software
This section describes how to compile the demo software on the ZCU104

## Compile the Application

- Set up the Linux host environment for cross-compilation
  ```bash
  source $PFM_DIR/../../../sysroot/environment-setup-aarch64-xilinx-linux
  export SYSROOT=$PFM_DIR/../../../sysroot/sysroots/aarch64-xilinx-linux
  ```  

  **Note:** When sourcing the environment setup script you may receive a warning indicating that you need to ``unset LD_LIBRARY_PATH``.  If that is the case then please execute ``unset LD_LIBRARY_PATH`` and then source the environment setup script again.

- Download the demo package from the Xilinx download site - <a href="https://www.xilinx.com/member/forms/download/design-license-zcu104-vcu-8channel.html?filename=zcu104_vcu_ml_2019_2_demo.zip">link</a>.

- Extract the demo archive to the project directory
  ```bash
  unzip zcu104_vcu_ml_2019_2_demo.zip -d $PROJ_DIR/zcu104_vcu_ml_2019_2_demo
  ```

- Several make files will need to be modified to point to your cross-compilation root file system location
  ```txt
  zcu104_vcu_ml_2019_2_demo/workspaces/gst/allocator/Debug/makefile
  zcu104_vcu_ml_2019_2_demo/workspaces/gst/allocator/Debug/subdir.mk
  zcu104_vcu_ml_2019_2_demo/workspaces/gst/base/Debug/makefile
  zcu104_vcu_ml_2019_2_demo/workspaces/gst/base/Debug/subdir.mk
  zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxfacedetect/Debug/makefile
  zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxfacedetect/Debug/src/subdir.mk
  zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxtrafficdetect/Debug/makefile
  zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxtrafficdetect/Debug/subdir.mk  
  zcu104_vcu_ml_2019_2_demo/workspaces/rtsp/makefile
  zcu104_vcu_ml_2019_2_demo/workspaces/rtsp/subdir.mk
  zcu104_vcu_ml_2019_2_demo/workspaces/xrtutils/Debug/makefile
  zcu104_vcu_ml_2019_2_demo/workspaces/xrtutils/Debug/subdir.mk  
  ```

  <br>
  In the files listed above you will see several hard coded paths that need to be updated.  

  <br>
  
  Modified make files can be created by the ``update_make.sh`` script to modify files to use the ``$SYSROOT`` environment variable
  
  ```bash
  cd $PROJ_DIR/zcu104_vcu_ml_2019_2_demo/workspaces
  bash -x $PROJ_DIR/zcu104-vcu-ml-build-example/scripts/update_make.sh
  ```

- Compile the demo application
  * The ``$PROJ_DIR/zcu104-vcu-ml-build-example/scripts/compile_all.sh`` script can be used to compile the demo application  
    ```bash
    bash -x $PROJ_DIR/zcu104-vcu-ml-build-example/scripts/compile_all.sh
    ```

  * If you want to manually compile each library component then you will need to do so in the following order
    1) zcu104_vcu_ml_2019_2_demo/workspaces/xrtutils
    2) zcu104_vcu_ml_2019_2_demo/workspaces/gst/allocator
    3) zcu104_vcu_ml_2019_2_demo/workspaces/gst/base
    4) zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxfacedetect
    5) zcu104_vcu_ml_2019_2_demo/workspaces/gstsdxtrafficdetect
    6) zcu104_vcu_ml_2019_2_demo/workspaces/rtsp
    <br>

## Compile the Face and Traffic Models
- Download the face detection and traffic detection models from the Vitis AI Model Zoo
  ```bash
  mkdir -p $PROJ_DIR/models
  cd $PROJ_DIR/models
  wget https://www.xilinx.com/bin/public/openDownload?filename=cf_densebox_wider_320_320_1.1.zip -O cf_densebox_wider_320_320_1.1.zip
  wget https://www.xilinx.com/bin/public/openDownload?filename=cf_ssdtraffic_360_480_0.9_1.1.zip -O cf_ssdtraffic_360_480_0.9_1.1.zip
  unzip cf_densebox_wider_320_320_1.1.zip 
  unzip cf_ssdtraffic_360_480_0.9_1.1.zip 
  ```
- Modify the ``cf_ssdtraffic_360_480_0.9_11.6G/quantized/deploy.prototxt`` file to specify a new input layer
  * Replace
    ```txt
    name:"test"
    input:"data"
    input_shape{
    dim:1
    dim:3
    dim:360
    dim:480
    }
    ```
  * With
    ```txt
    layer {
      name: "data"
      type: "Input"
      top: "data"
      transform_param {
        mean_value: 104
        mean_value: 117
        mean_value: 123
        force_color: true
        resize_param {
          prob: 1
          resize_mode: WARP
          height: 360
          width: 480
          interp_mode: LINEAR
        }
      }
      input_param {
        shape {
          dim: 1
          dim: 3
          dim: 360
          dim: 480
        }
      }
    }
    ```

- Remove the following unsupported output layers
  + ``mbox_conf_reshape``
  + ``mbox_conf_softmax``
  + ``mbox_conf_flatten``
  + ``detection_out``

- Compile the models for execution on the DPU
  * If you don't have docker set up on your host machine follow the instructions <a href="https://github.com/Xilinx/Vitis-AI/blob/master/doc/install_docker/README.md">here</a>  
  
  * Start the docker container
    ```bash
    cd $PROJ_DIR
    ~/Vitis-AI-v1.1/docker_run.sh xilinx/vitis-ai-cpu:latest
    ```
  
  * Load the Caffe conda environment within the docker container
    ```bash
    conda activate vitis-ai-caffe
    ```

  * Create a JSON file which specifies the DPU architecture options
    ```bash
    dlet -f DPU-TRD/prj/Vitis/binary_container_1/sd_card/zcu104_vcu_ml.hwh
    mv *.dcf models/zcu104_vcu_ml.dcf
    cd models
    echo '{"target":"dpuv2", "dcf":"zcu104_vcu_ml.dcf", "cpu_arch":"arm64"}' > zcu104_vcu_ml.json 
    ```
  
  * Compile the face detection model
    ```bash
    vai_c_caffe --prototxt cf_densebox_wider_320_320_0.49G/quantized/deploy.prototxt \
                --caffemodel cf_densebox_wider_320_320_0.49G/quantized/deploy.caffemodel \
                --arch zcu104_vcu_ml.json \
                --output_dir vai_c_output_face/ \
                --net_name densebox \
                --options "{'save_kernel':''}"
    
    ```
  
  * Compile the traffic detection model
    ```bash
    vai_c_caffe --prototxt cf_ssdtraffic_360_480_0.9_11.6G/quantized/deploy.prototxt \
                --caffemodel cf_ssdtraffic_360_480_0.9_11.6G/quantized/deploy.caffemodel \
                --arch zcu104_vcu_ml.json \
                --output_dir vai_c_output_traffic/ \
                --net_name ssd \
                --options "{'save_kernel':''}"
    ```  

  * Exit the docker container by pressing ``ctrl-d``

  * Convert the DPU executable files to shared libraries
    ```bash
    cd $PROJ_DIR/models
    aarch64-xilinx-linux-g++ -nostdlib -fPIC -shared vai_c_output_face/dpu_densebox.elf -o libdpumodeldensebox.so
    aarch64-xilinx-linux-g++ -nostdlib -fPIC -shared vai_c_output_traffic/dpu_ssd.elf -o libdpumodelssd.so
    ```

# Part 4: Running on the ZCU104 target hardware
This section describes how to compile the demo software on the ZCU104


- Copy files to an ``sd_card`` directory
  * You can do this by using the ``copy_sd_files.sh`` script 
    ```bash
    cd $PROJ_DIR/zcu104_vcu_ml_2019_2_demo
    bash -x $PROJ_DIR/zcu104-vcu-ml-build-example/scripts/copy_sd_files.sh
    ```

    The files will be located in an sd_card directory at ``$PROJ_DIR/zcu104_vcu_ml_2019_2_demo``
  
  * If you want to copy the files manually then see the ``copy_sd_files.sh`` script for reference

    **Note:** There are several scripts in the pre-built sdcard directory included in the ``zcu104_vcu_ml_2019_2_demo.zip`` archive that are copied to the ``sd_card`` directory.

- Copy files to the SD card

    + Format the SD card with 2 partitions using the instructions - <a href="https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842385/How+to+format+SD+card+for+SD+boot">here</a>
    + Copy the contents of the ``$PROJ_DIR/zcu104_vcu_ml_2019_2_demo/sd_card`` directory to the BOOT partition of the SD card.  The following command assumes the BOOT partition is mounted at ``/media/BOOT`` on your Linux host, modify accordingly
      
      ```bash
      cp -rp $PROJ_DIR/zcu104_vcu_ml_2019_2_demo/sd_card/* /media/BOOT/.
      ```

    + Extract the root file system to the ROOT partition of the SD card.  The following command assumes the root partition is mounted at ``/media/ROOT`` on your Linux host, modify accordingly

      ```bash
      sudo tar -xvzf $PROJ_DIR/zcu104_vcu_ml_2019_2_demo/sd_card/rootfs.tar.gz -C /media/ROOT
      ```

    + Unmount and eject the SD card from your Linux host

 - Insert the SD card into the ZCU104 and boot the board
  
   **Note:** This demo uses the HDMI Tx port on the board for displaying results - make sure your hardware is connected properly

- Once the ZCU104 boots, navigate to the ``/media/card`` directory and execute the ``setup.sh`` script
  ```txt
  ZCU104: cd /media/card
  ZCU104: ./setup.sh
  ```

- Run an 8-channel face & traffic detection example
  ```txt
  ZCU104: ./8_ch_traffic_face.sh
  ```

