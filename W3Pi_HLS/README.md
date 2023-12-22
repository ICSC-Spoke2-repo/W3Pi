# W3Pi_HLS
HLS implementation of the $W\to3\pi$ CMS Phase-2 analysis. <br>
Largely based on excercise 2 from the ICSC Spoke-2 tutorial: [Introductory course to HLS FPGA programming](https://agenda.infn.it/event/38191/timetable/?view=standard#b-34259-hls-programming).

## Main concept
Build several "kernels" to perform _online_ the $W\to3\pi$ scouting analysis:
* `analysis main`:
  * Read stream of input data
  * Do the unpacking
  * Initial selection (per event)
* `event_processor`:
  * Read Puppi candidates in one event
  * Filter them
  * Create all possible triplets starting from the pivot (highest pT)
  * Dilter the triplets according to some selections
  * Pass them to the next kernel
* `DNN inference`:
  * For each triplet run the DNN inference exploiting the _HLS queue distribution_

## Implementation Status

- [ ] `analysis main`
- [x] `event_processor` (not yet optimized, neither for latency, nor for resource consumption)
- [ ] `DNN inference`
- [ ] linking of the kernels

## Directories Structure
* `data`: contains three input files
  * `Puppi_SingleNu.dump`: 17821 events of SingleNu (background)
  * `Puppi_w3p_PU0.dump`: 101 events of $W\to3\pi$ at PU 0 (signal)
  * `Puppi_w3p_PU200.dump`: 101 events of $W\to3\pi$ at PU 200 (signal)
  * All files have also a _.root_ version for double checking and debugging

* `event_processor`: contains the cpp/HLS code to be synthesized
  * Firmware code under `event_processor/src`
  * Testbench file: `event_processor/testbench.cc`
  * Vitis HLS project file: `event_processor/run_hls_w3p.tcl`

## How to run the code
For the moment, only the `event_processor` code is implemented, and it's still lacking optimization in terms of both latency and resource consumption.

To run the code:
* Connect to `cerere.mib.infn.it` and source the environment:
  ```
  ssh [username]@cerere.mib.infn.it
  source vitis-hls.sh
  ```
  * Note that this machine is _only_ visible from within the INFN Milano-Bicocca network and you need an account
* Clone the repository:
  ```
  git clone git@github.com:ICSC-Spoke2-repo/W3Pi.git
  ```
* Go in the HLS directory and run the code:
  ```
  cd W3Pi/W3Pi_HLS
  vitis_hls -f run_hls_w3p.tcl
  ```
  * By default only the _C simulation_ is running. To also run the synthesis uncomment the `csynth_design` line in `run_hls_w3p.tcl` (not it may take a while)
  * To run it interactively with the Vitis HLS GUI use `vitis_hls -p run_hls_w3p.tcl`
