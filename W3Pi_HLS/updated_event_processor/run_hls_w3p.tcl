# create a project
open_project -reset "proj_v1"

# specify the name of the function to synthetize
#set_top masker
#set_top slimmer
#set_top slimmer2
#set_top orderer
#set_top orderer2
#set_top orderer3
#set_top orderer4
#set_top orderer5
#set_top orderer6
#set_top orderer7
#set_top orderer7bis
#set_top orderer7c
#set_top orderer7d
#set_top orderer7e
#set_top orderer7f
#set_top orderer7g --> II Violation
#set_top orderer8
#set_top merger
#set_top merger7bis
#set_top merger7f
#set_top get_cos_phi
#set_top get_cosh_eta
set_top get_pair_mass
#set_top get_triplet_inputs
#set_top event_inputs
#set_top get_event_scores
#set_top get_highest_score
#set_top EventProcessor
#set_top EventProcessor7bis
#set_top EventProcessor7f

# load source code for synthesis
add_files src/event_processor.cc

# load source code for the testbench
#  - add `-cflags "-DON_W3P"` to testbench --> not sure what for
add_files -tb event_processor_ref.cc
add_files -tb testbench.cc
add_files -tb ../data/Puppi_w3p_PU200.dump
add_files -tb BDT/conifer_binary_featV4_finalFit_v5.json

# create a solution (i.e. a hardware configuration for synthesis)
#  - add `-flow_target vitis` for implementation on board (axi and gmem management)
open_solution -reset "solution"

# Alveo U50 and a 200 MHz clock
# 200 MHz --> period 5      ns
# 300 MHz --> period 3.3333 ns
# 360 MHz --> period 2.7778 ns
# 400 MHz --> period 2.5    ns
set_part {xcu50-fsvh2104-2-e}
create_clock -period 5

# Run
csim_design
#csynth_design
#export_design -flow syn -format xo
exit
