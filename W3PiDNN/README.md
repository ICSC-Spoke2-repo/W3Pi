# W3PiDNN

This package contains a minimal set of scripts needed to run a simple Fully Connected DNN for the W->3pions analysis.

# Installation
The [Mamba](https://mamba.readthedocs.io/en/latest/) package manager is used to install the correct environment for running the W3PiDNN package.

To ease the installation, the dictionary ([w3pDNN-env-snapshot.yaml](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/w3pDNN-env-snapshot.yaml)) provided in this repository contains all the required packages and version explicitly mentioned

Simple recipe for the installation:
```bash
git clone git@github.com:ICSC-Spoke2-repo/W3Pi.git
cd W3PiDNN
mamba env create --file w3pDNN-env-snapshot.yaml
mamba activate w3pDNN
mamba env config vars set LD_LIBRARY_PATH=$(realpath $(dirname $CONDA_EXE)/../lib)
mamba deactivate
mamba activate w3pDNN
```

The environment is now ready and can be easily activated with:
```bash
mamba activate w3pDNN
```

# How to run the code
1. Create an hdf file to be used in the DNN training using [create_h5.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/create_h5.py). <br> Example command:
   ```python
   python3 create_h5.py \
     --inputS my_input_signal_file_1.root my_input_signal_file_2.root \
     --inputB my_input_background_file_1.root \
     --output hdf_files/hdf_my_prepared_file.h5 \
     --threads 6 \
     --features config/setup_v1.py
   ```
   1. Plot the features from the hdf file with [plot_h5.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/plot_h5.py). <br> Example command:
      ```python
      python3 plot_h5.py
      ```
      (no argparse in this script, please modify the `version` string in the script to pick up the correct hdf file)

2. Run the DNN training with [FC_training_w3p_v1.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/FC_training_w3p_v1.py). <br> Example command:
   ```python
   python3 FC_training_w3p_v1.py \
     --input  hdf_files/hdf_my_prepared_file.h5 \
     --output trainings/w3pDNN_v1 \
     --setup  config/setup_v1.py \
     --neurons 37 35 30 25 30 35 \
     --cpu
   ```

3. Create samples with NN prediction with [FC_evaluate_w3p_v1_signal.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/FC_evaluate_w3p_v1_signal.py) and [FC_evaluate_w3p_v1_background.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/FC_evaluate_w3p_v1_background.py). <br> Example command:
   ```python
   time python3 FC_evaluate_w3p_v1_signal.py \
     --input trainings/w3pDNN_v20 \
     --maxN 10000
   ```
   and
   ```python
   time python3 FC_evaluate_w3p_v1_background.py \
     --input trainings/w3pDNN_v20 \
     --maxN 100000
   ```

4. Compute efficiencies (selection and purity) on predicted samples with [read_predicted_h5.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/read_predicted_h5.py), <br> Example command:
   ```python
   python3 read_predicted_h5.py \
     -v 20 \
     -l _p2
   ```
   and plot the highest triplet NN score with [plot_predicted_score.py](https://github.com/ICSC-Spoke2-repo/W3Pi/blob/master/W3PiDNN/plot_predicted_score.py).
   ```python
   python3 plot_predicted_score.py
   ```
   (no argaparse in this script, please modify the `training version` string in the script to pick up the correct training)

# Models Available

1. **FC_training_w3p_v1:**
   Simple Fully Connected DNN.

   Inputs:
   - All possible triplets in the event

   Output:
   - A score (between 0 and 1) for each triplet: highest the score, highest the probability to be the W->3Pi triplet
