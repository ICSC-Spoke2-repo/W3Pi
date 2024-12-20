# Create a project
open_project -reset "proj_w3p_streamer"

# Specify the name of the top function to synthetize
set_top w3p_streamer

# Load source code for synthesis
add_files src/w3p_streamer.cc

# Load source code for the testbench
#  - add `-cflags "-DON_W3P"` to testbench --> not sure what for
add_files -tb src/w3p_emulator.cc
add_files -tb testbench_w3p_streamer.cc
add_files -tb ../data/Puppi_w3p_PU200_a.dump
add_files -tb ../data/Puppi_w3p_PU200_b.dump
add_files -tb ../data/Puppi_w3p_PU200_c.dump
add_files -tb ../data/Puppi_w3p_PU200_d.dump

# Create a solution (i.e. a hardware configuration for synthesis)
#  - add `-flow_target vitis` for implementation on board (axi and gmem management)
open_solution "solution" -flow_target vitis

# Set board:
# Alveo U50 --> xcu50-fsvh2104-2-e
# VUP9      --> xcvu9p-flga2577-2-e
set_part {xcu50-fsvh2104-2-e}

# Set clock
# 200 MHz --> period 5      ns
# 300 MHz --> period 3.3333 ns
# 360 MHz --> period 2.7778 ns
# 400 MHz --> period 2.5    ns
create_clock -period 5

# Run
csim_design
csynth_design
#cosim_design
#export_design -flow syn -format xo
#export_design -flow impl -format xo
#export_design -flow impl -format ip_catalog -rtl vhdl
exit
