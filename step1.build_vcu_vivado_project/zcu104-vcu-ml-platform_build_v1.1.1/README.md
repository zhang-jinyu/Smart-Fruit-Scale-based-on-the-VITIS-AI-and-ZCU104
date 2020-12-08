<p>
<div align="center">
  <h1 align="center">ZCU104 VCU + ML Platform & Build</h1>
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

- If you would like to use the Vitis-AI Runtime and Vitis-AI Library then you will need to add additional packages.
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
