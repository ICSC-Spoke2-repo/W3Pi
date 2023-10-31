# General imports
import os
import sys ; sys.path.append(os.getcwd())
import argparse
import time

# ROOT and utils imports
import ROOT
import pandas as pd
import numpy as np

# Keras and TF imports
import tensorflow as tf

# Custom imports
from config.setup_v1 import *
from utils.RootDF_utils import *
from utils.PandasDF_utils import *

# Parse arguments
parser = argparse.ArgumentParser('Simple Script for the w3pDNN Evaluation.\n\
Pass different arguments in the form --some-setup arg1='"string_val"' args2=float_val (note the multiple quotes for strings)'
)
parser.add_argument('-i', '--input', required=True,          help='Input training directory, e.g. trainings/w3pDNN_v1'  )
parser.add_argument('-n', '--maxN' , default=-1   , type=int, help='Max entries on which running the inference'  )

args = parser.parse_args()

# Example commands
'''
time python3 FC_evaluate_w3p_v1_signal.py \
  --input trainings/w3pDNN_v20 \
  --maxN 10000

time python3 FC_evaluate_w3p_v1_signal.py \
  --input prunings/w3pDNN_v20_p1 \
  --maxN 10000
'''

start_time = time.time()

# ----- Load trained model -----
# Load model
model_name = args.input
my_model = tf.keras.models.load_model(model_name)

# Print a summary of the loaded model
my_model.summary()
model_time = time.time()

# ----- Evaluate on a sample -----
# Read input data from ROOT file
infile = '/gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/v1/l1Nano_WTo3Pion_PU200.v1_more.root'
#infile = '/gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/v1/l1Nano_WTo3Pion_PU200.v1.root'

print('Evaluating model {} on file: {}'.format(model_name, infile))

# Define RDF
frame = ROOT.RDataFrame('Events', infile)
print('Initial entries :', frame.Count().GetValue())

# Apply minimal selection
selection = 'nL1Puppi >= 3'
#selection = '(event == 1 && (luminosityBlock == 1000 || luminosityBlock == 1013 || luminosityBlock == 1009 || luminosityBlock == 1012 || luminosityBlock == 1002 || luminosityBlock == 1016 || luminosityBlock == 1023)) || (luminosityBlock == 1000 && event == 3)'
frame = frame.Filter(selection, 'minimal selection')
print('Selected entries:', frame.Count().GetValue())
if args.maxN > 0:
    frame=frame.Range(args.maxN)
print('Ranged entries  :', frame.Count().GetValue())

# Add Gen-matching to RDF
frame = add_genmatched(frame)
frame = frame.Filter('genmatched')
frame = frame.Define('reco_matched_idxs', 'add_candidate_matched_idxs(L1Puppi_GenPiIdx)')
print('Genmatched entries  :', frame.Count().GetValue())
select_filter_time = time.time()

# Add flag for gen-matched events within detector acceptance
frame = add_gen_acceptance(frame)

# Prepare Root DF
frame = prepare_inference_df(frame, OUT_BRANCHES)
frame = frame.Define('myrow', 'rdfentry_')
print('Prepared RDF')
prepareRDF_time = time.time()

# Transform in Pandas DF to run inference 
frame_numpyzed = frame.AsNumpy()
print('Numpyzed RDF')
numpyze_time = time.time()

pdframe = pd.DataFrame.from_dict(frame_numpyzed)
print('Transformed into PandasDF')
pandasDF_time = time.time()

# Loop on each event to get NN scores for each triplet for each event
all_predictions_map = {}
for index, event in pdframe.iterrows():

    # Get inputs to evaluate NN from each triplet
    inputs = get_triplets_inputs(event)

    # Get precions for each triplet
    predictions = []
    for inp in inputs:
        predictions.append( my_model(inp) )

    predictions = np.concatenate(predictions).reshape((len(predictions),))

    all_predictions_map[event.myrow] = predictions

print('Predictions computed')
predictions_time = time.time()

# ----- Evaluate targets -----
# Targets:
#  - Gen matching efficiency for signal
#  - Estimate of background rejection efficiency (target should be around 1e-9)

# Define to ROOT a map with the scores
scoremap_snippet = '''
std::map<int, std::vector<float>> scoremap;
{}
'''.format(
    '\n'.join([
            'scoremap.insert({%s,{%s}});' % (
                str(key), ','.join([str(pred) for pred in all_predictions_map[key]])
            ) for key in list(all_predictions_map.keys())
        ])
    )
ROOT.gInterpreter.ProcessLine(scoremap_snippet)

# Add scores to the RDF
frame = frame.Define('triplet_scores', 'scoremap[myrow];')
print('Added predictions to RDF')
add_predictions_time = time.time()

# add size of triplets
frame = frame.Define('triplets_size', 'triplet_idxs.size()')

# add highest triplet score, score idx, predicted pions idxs 
frame = frame.Define('highest_score_idx', 'std::max_element(triplet_scores.begin(),triplet_scores.end()) - triplet_scores.begin()')
frame = frame.Define('highest_score', 'triplet_scores[highest_score_idx]')
predicted_idxs = '''
    std::vector<int> my_predicted_idxs {triplet_idxs[highest_score_idx].idx0,triplet_idxs[highest_score_idx].idx1,triplet_idxs[highest_score_idx].idx2};
    return my_predicted_idxs;
'''
frame = frame.Define('predicted_idxs', predicted_idxs)

# Check how many times the prediction is true (predicted idxs match the gen idxs)
frame = frame.Define('predicted_true', 'std::is_permutation( predicted_idxs.begin(), predicted_idxs.end(), reco_matched_idxs.begin() )')
print("predicted_true entries:", frame.Filter('predicted_true').Count().GetValue())
predicted_true_time = time.time()

# ----- Save the RDF to HDF with predictions added -----
np_to_save =  frame.AsNumpy()
oframe = pd.DataFrame.from_dict(np_to_save)
oframe = oframe.drop(columns=['triplet_idxs'])
oframe.to_hdf('hdf_files/predicted/hdf_'+model_name.split('/')[1]+'_signalPU200_small.h5', key='df')
saveHDF_time = time.time()

# ----- Print timing information -----
timing_string = '''
---- Timing evaluations ----
 - Model loading         : {:.4} seconds, from start: {:.4} seconds
 - selection/filter/range: {:.4} seconds, from start: {:.4} seconds
 - Prepare RootD         : {:.4} seconds, from start: {:.4} seconds
 - Numpyze               : {:.4} seconds, from start: {:.4} seconds
 - Prepare PandasDF      : {:.4} seconds, from start: {:.4} seconds
 - Get predictions map   : {:.4} seconds, from start: {:.4} seconds
 - Add predictions       : {:.4} seconds, from start: {:.4} seconds
 - Predicted true        : {:.4} seconds, from start: {:.4} seconds
 - Save h5 file          : {:.4} seconds, from start: {:.4} seconds
'''.format(
    (model_time - start_time)                   , (model_time - start_time),
    (select_filter_time - model_time)           , (select_filter_time - start_time),
    (prepareRDF_time - select_filter_time)      , (prepareRDF_time - start_time),
    (numpyze_time - prepareRDF_time)            , (numpyze_time - start_time),
    (pandasDF_time - numpyze_time)              , (pandasDF_time - start_time),
    (predictions_time - pandasDF_time)          , (predictions_time - start_time),
    (add_predictions_time - predictions_time)   , (add_predictions_time - start_time),
    (predicted_true_time - add_predictions_time), (predicted_true_time - start_time),
    (saveHDF_time - predicted_true_time)        , (saveHDF_time - start_time)
)
print(timing_string)

#import pdb; pdb.set_trace()

'''

cols = ('candidate_idxs','reco_matched_idxs', 'genmatched', 'triplet_idxs', 'triplet_scores', 'triplets_size', 'highest_score_idx', 'highest_score', 'predicted_idxs')
print_dataframe(frame, cols)

'''


