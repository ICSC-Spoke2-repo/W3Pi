# General imports
import os
import sys ; sys.path.append(os.getcwd())
import argparse

# Keras and TF imports
import tensorflow as tf
from keras.callbacks import TensorBoard, ModelCheckpoint

# Feature importance imports
from sklearn.inspection import permutation_importance
import matplotlib.pyplot as plt

# Custom imports
from callbacks.plotting_w3p_v1 import w3pPlotCallback, w3pPlotCallback2
from config.FCModel import FCModel

# Parse arguments
parser = argparse.ArgumentParser('Simple Dense Sequence NN for the w3pDNN training.\n\
Pass different arguments in the form --some-setup arg1='"string_val"' args2=float_val (note the multiple quotes for strings)'
)
parser.add_argument('-i', '--input'     , required=True  , nargs='+'            , help='List of input .h5 files (space separated)'  )
parser.add_argument('-o', '--output'    , required=True                         , help='Output trained file'                        )
parser.add_argument('-s', '--setup'     , required=True                         , help='Load the setup from a python script'        )
parser.add_argument('-d', '--dropout'   , default=None                          , help='use dropout for the hidden layers (None=no)')
parser.add_argument('-n', '--neurons'   , default=[15]*3 , nargs='+', type=int  , help='list of neuron numbers (per layer)'         )
parser.add_argument('-N', '--name'      , default='w3pDNN'                      , help='Model name'                                 )
parser.add_argument('-D', '--dryrun'    , action='store_true'                   , help='Don\'t fit'                                 )
parser.add_argument('-F', '--fast'      , action='store_true'                   , help='Don\'t run plotting callbacks'              )
parser.add_argument(      '--cpu'       , action='store_true'                   , help='Run on CPU'                                 )
parser.add_argument(      '--feat_imp'  , action='store_true'                   , help='Get Feature importance with PMI'            )
parser.add_argument(      '--scoring'   , default='explained_variance'          , help='Scorer for PMI (r2,explained_variance)'     )
args = parser.parse_args()

# Example commands
'''
time python3 FC_training_w3p_v1.py \
  --input  hdf_files/hdf_nL1Puppi_3_v4_pivot_shuffle_large.h5 \
  --output trainings/w3pDNN_v1 \
  --setup  config/setup_v1.py \
  --neurons 37 35 30 25 30 35 \
  --cpu \
  --feat_imp

time python3 FC_training_w3p_v1.py \
  --input  hdf_files/hdf_nL1Puppi_3_pivot_pt_ordered_large.h5 \
  --output trainings/w3pDNN_v2 \
  --setup  config/setup_v1.py \
  --neurons 37 35 30 25 30 35 \
  --cpu \
  --feat_imp

'''

# Assert if output folder already exists
assert not os.path.exists(args.output), "Output folder alredy exists, will not proceed"

# Set CPU/GPU usage
if args.cpu:
  os.environ['CUDA_VISIBLE_DEVICES']=''

# Declare an FCModel model
model = FCModel(
  name      = args.name   ,
  files     = args.input  ,
  output    = args.output ,
  setup     = args.setup  ,
  neurons   = args.neurons,
  dropout   = args.dropout,
  n_targets = 1           ,
)

# Load and compile model
model.load()
model.compile()

# Define the callbacks
callbacks = [ # Standard callbacks
  TensorBoard(
    log_dir = model.log_dir
  ),
  ModelCheckpoint(
    filepath  = args.output+'/checkpoints/epoch_{epoch:02d}', 
    save_freq = 10
  ),
]+[ # Custom callbacks
  w3pPlotCallback(
    name    = 'signal'       , 
    data    = model.x_test[model.y_test == 1] ,
    log_dir = model.log_dir
  )
]+[
  w3pPlotCallback(
    name    = 'background'       , 
    data    = model.x_test[model.y_test == 0] ,
    log_dir = model.log_dir
  )
]+[
  w3pPlotCallback2(
    name            = 'overlayed',
    data_signal     = model.x_test[model.y_test == 1],
    data_background = model.x_test[model.y_test == 0],
    log_dir         = model.log_dir,
  )
]

# Fit Model with callbacks
if not args.dryrun:
  if args.fast:
    print("Fast fitting (no callbacks)!")
    model.fit()
  else:
    print("Fitting with callbacks!")
    model.fit(callbacks=callbacks)
else:
  print("This is just a dry-run! No fitting performed!")

# Calculate the feature importance scores
if args.feat_imp:
  # Get importances from permutations
  results = permutation_importance(model.model, model.x_test, model.y_test, n_repeats=10, random_state=42, scoring=args.scoring)
  importance = results.importances_mean

  # Print the feature importance scores
  for i,v in enumerate(importance):
    print('Feature %d: %.5f' % (i,v))

  # Plot the feature importance chart
  plt.bar(model.FEATURES, importance)
  plt.xticks(rotation=90)
  plt.subplots_adjust(left=0.1, right=0.95, top=0.95, bottom=0.25)
  plt.savefig(args.output+'/feature_importance_'+args.scoring+'.png')
  plt.close()

