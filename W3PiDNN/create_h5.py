#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# General Import
import os
import sys ; assert sys.hexversion>=((3<<24)|(7<<16)), "Python 3.7 or greater required" # a binary joke is worth 111 words
sys.path.append(os.getcwd())

import ROOT
import pandas as pd
import numpy as np
import math
import argparse

# Specific imports
from utils.RootDF_utils import *
from utils.PandasDF_utils import *

# Arg Parser
parser = argparse.ArgumentParser('Build .h5 files from a list of .root files for the w3pDNN.\n\
NOTE: the test sample size is deduced from the --train-size and --valid-size arguments')
parser.add_argument('--inputS'    , required=True, nargs='+'  , help='List of the input signal .root file')
parser.add_argument('--inputB'    , required=True, nargs='+'  , help='List of the input background .root file')
parser.add_argument('--output'    , required=True             , help='Output .h5 file')
parser.add_argument('--features'  , required=True             , help='Python files with the FEATURES dictionary')
parser.add_argument('--tree'      , default='Events'          , help='Tree name')
parser.add_argument('--target'    , default='class'           , help='Target variable name')
parser.add_argument('--train-size', default=0.65 , type=float , help='Fraction of the train sample')
parser.add_argument('--valid-size', default=0.25 , type=float , help='Fraction of the validation sample')
parser.add_argument('--threads'   , default=1    , type=int   , help='Number of threads')
args = parser.parse_args()

'''
SIGNALS
  /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/125-v0/l1Nano_WTo3Pion_PU0.125X_v0.root
  /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/125-v0/l1Nano_WTo3Pion_PU200.125X_v0.root
  /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/v1/l1Nano_WTo3Pion_PU0.v1.root         --> missing bunchCrossing branch + contain additional L1PF branches
  /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/v1/l1Nano_WTo3Pion_PU200.v1_more.root  --> missing bunchCrossing branch + contain additional L1PF branches
  /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/v1/l1Nano_WTo3Pion_PU200.v1.root       --> missing bunchCrossing branch + contain additional L1PF branches

time python3 create_h5.py \
  --inputS /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/125-v0/l1Nano_WTo3Pion_PU200.125X_v0.root /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/v1/l1Nano_WTo3Pion_PU200.v1.root \
  --inputB /gwteraz/users/brivio/l1scouting/w3pi/ntuples-Giovanni/125-v0/l1Nano_SingleNeutrino_PU200.125X_v0.root \
  --output hdf_files/hdf_nL1Puppi_3_v4_pivot_shuffle_large.h5 \
  --threads 6 \
  --features config/setup_v1.py
'''

# ROOT Multithreading based on argparse
if args.threads > 1:
  ROOT.ROOT.EnableImplicitMT(args.threads)

# Import branches
NEEDED_BRANCHES     = __import__(args.features.replace('/', '.').strip('.py'), fromlist=['']).NEEDED_BRANCHES
GEN_NEEDED_BRANCHES = __import__(args.features.replace('/', '.').strip('.py'), fromlist=['']).GEN_NEEDED_BRANCHES
OUT_BRANCHES        = __import__(args.features.replace('/', '.').strip('.py'), fromlist=['']).OUT_BRANCHES

# ---------------------------------
# Apply Minimal selection on input events
#baseline = 'nL1Puppi >= 3'
#baseline = 'luminosityBlock == 1000 && event == 1'
#baselineS = '(event == 1 && (luminosityBlock == 1000 || luminosityBlock == 1013 || luminosityBlock == 1009 || luminosityBlock == 1012 || luminosityBlock == 1002 || luminosityBlock == 1016 || luminosityBlock == 1023)) || (luminosityBlock == 1000 && event == 3)'
#baselineB = '(run==1 && event==1566242 && luminosityBlock==9807) || (run==1 && event==1566243 && luminosityBlock==9807) || (run==1 && event==1566244 && luminosityBlock==9807) || \
#(run==1 && event==1566245 && luminosityBlock==9807) || (run==1 && event==1566246 && luminosityBlock==9807) || (run==1 && event==1566247 && luminosityBlock==9807)'

baseline = 'nL1Puppi >= 3'
baselineS = baseline
baselineB = baseline

# Define Signal and background RDF
sframe = ROOT.RDataFrame(args.tree, args.inputS)
bframe = ROOT.RDataFrame(args.tree, args.inputB)

# Check all needed branches are present
# FIXME: add assert based on 'NEEDED_BRANCHES' and 'GEN_NEEDED_BRANCHES'

# Filter RDF
sframe = sframe.Filter(baselineS)
bframe = bframe.Filter(baselineB)

# Make NumPy Root DF from signal and background, includes:
# - process the RDFs to add the correct branches (OUT_BRANCHES)
# - force B to have same num of events of S
# - returns 2 "numpyzed" DFs, ready to be merged and passed to Pandas
sframe_numpyzed, bframe_numpyzed = make_numpy_from_RootDF(sframe, bframe, OUT_BRANCHES)

print(' > sframe_numpyzed size:', sframe_numpyzed['event'].size)
print(' > bframe_numpyzed size:', bframe_numpyzed['event'].size)

# Make Pandas DF from numpyzed Root DFs to dump in HD5 file
oframe = make_PandasDF(sframe_numpyzed, bframe_numpyzed)
print(' > oframe size:', oframe['event'].size)

# Recast PandasDF types (needed by Pandas)
oframe = recast_PandasDF(oframe, OUT_BRANCHES)
print(' > recast size:', oframe['event'].size)

# Add values to train/validation/test columns
oframe = train_test_valid_split(oframe, args.train_size, args.valid_size)
print(' > ttv size   :', oframe['event'].size)

# Dump to HDF
oframe.to_hdf(args.output, key='df')
