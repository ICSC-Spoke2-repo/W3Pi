open_project -reset proj_w3p
set_top event_processor
add_files src/event_processor.cc
add_files -tb event_processor_ref.cc
add_files -tb testbench.cc -cflags "-DON_W3P"
add_files -tb ../data/Puppi_w3p_PU200.dump

open_solution -reset "solution"
set_part {xcvu9p-flga2577-2-e}
create_clock -period 2.777

csim_design
#csynth_design
exit
