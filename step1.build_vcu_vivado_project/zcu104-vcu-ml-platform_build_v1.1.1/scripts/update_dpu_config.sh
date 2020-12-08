#!/bin/bash

# Modify the DPU configuration to enable URAM
sed -i 's/^`define URAM_DISABLE/`define URAM_ENABLE/' dpu_conf.vh

# Modify the project configuration file to specify 1 DPU kernel
cp config_file/prj_config config_file/prj_config.orig
sed -i '/freqHz=300000000:dpu_xrt_top_2.aclk/d' config_file/prj_config
sed -i '/freqHz=600000000:dpu_xrt_top_2.ap_clk_2/d' config_file/prj_config
sed -i '/sp=dpu_xrt_top_2.M_AXI_GP0:HPC0/d' config_file/prj_config
sed -i '/sp=dpu_xrt_top_2.M_AXI_HP0:HP2/d' config_file/prj_config
sed -i '/sp=dpu_xrt_top_2.M_AXI_HP2:HP3/d' config_file/prj_config
sed -i 's/nk=dpu_xrt_top:2/nk=dpu_xrt_top:1/' config_file/prj_config

# Update the DPU connections to be consistent with the original zcu104_vcu_ml demo
sed -i 's/sp=dpu_xrt_top_1.M_AXI_GP0:HPC0/sp=dpu_xrt_top_1.M_AXI_GP0:HP2/' config_file/prj_config
sed -i 's/sp=dpu_xrt_top_1.M_AXI_HP0:HP0/sp=dpu_xrt_top_1.M_AXI_HP0:HP2/' config_file/prj_config
sed -i 's/sp=dpu_xrt_top_1.M_AXI_HP2:HP1/sp=dpu_xrt_top_1.M_AXI_HP2:HP3/' config_file/prj_config
sed -i 's/#param=compiler.skipTimingCheckAndFrequencyScaling=1/param=compiler.skipTimingCheckAndFrequencyScaling=1/' config_file/prj_config

