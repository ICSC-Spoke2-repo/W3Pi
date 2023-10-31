import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import argparse

# ----- Parse arguments -----
parser = argparse.ArgumentParser('Simple Script for reading the w3pDNN predicted samples.\n\
Pass different arguments in the form --some-setup arg1='"string_val"' args2=float_val (note the multiple quotes for strings)'
)
parser.add_argument('-v', '--NNversion'  , required=True, type = int, help='NN model version' )
parser.add_argument('-l', '--NNlabel'    , default=''   ,             help='additional label for NN version' )

args = parser.parse_args()

'''
python3 read_predicted_h5.py \
  -v 20 \
  -l _p2
'''

# training version
version = args.NNversion
additional_label = args.NNlabel
versionName = 'w3pDNN_v'+str(version)+additional_label+'_signalPU200_small'

#------
def getEff (data, threshold):
    overT_data = data[data['highest_score'] >= threshold]
    n_overT_data = overT_data['run'].size
    return n_overT_data

def getPurity (data, threshold):
    overT_data = data[data['highest_score'] >= threshold]
    matched_data = overT_data[overT_data['predicted_true'] == 1]
    n_matched_data = matched_data['run'].size
    return n_matched_data
#------

# Read data
# signal
dataS = pd.read_hdf('hdf_files/predicted/hdf_'+versionName+'.h5')
dataS = dataS[dataS['genmatched']]          # 3 gen-matched pions
if version > 6:
    dataS = dataS[dataS['gen_acceptance']]  # 3 gen-matched pions + detector acceptance
# background
dataB = pd.read_hdf('hdf_files/predicted/hdf_'+versionName+'_background.h5')

# Denominator
tot_events_S = dataS['run'].size
tot_events_B = dataB['run'].size
original_events_B = 100000

# Numerator
# signal: denominator + highest score > Threshold
n_overT_0p1_S   = getEff(dataS,0.1)
n_overT_0p3_S   = getEff(dataS,0.3)
n_overT_0p5_S   = getEff(dataS,0.5)
n_overT_0p7_S   = getEff(dataS,0.7)
n_overT_0p8_S   = getEff(dataS,0.8)
n_overT_0p9_S   = getEff(dataS,0.9)
n_overT_0p95_S  = getEff(dataS,0.95)
n_overT_0p96_S  = getEff(dataS,0.96)
n_overT_0p97_S  = getEff(dataS,0.97)
n_overT_0p98_S  = getEff(dataS,0.98)
n_overT_0p99_S  = getEff(dataS,0.99)
n_overT_0p995_S = getEff(dataS,0.995)
n_overT_0p999_S = getEff(dataS,0.999)

n_overT_0p1_B   = getEff(dataB,0.1)
n_overT_0p3_B   = getEff(dataB,0.3)
n_overT_0p5_B   = getEff(dataB,0.5)
n_overT_0p7_B   = getEff(dataB,0.7)
n_overT_0p8_B   = getEff(dataB,0.8)
n_overT_0p9_B   = getEff(dataB,0.9)
n_overT_0p95_B  = getEff(dataB,0.95)
n_overT_0p96_B  = getEff(dataB,0.96)
n_overT_0p97_B  = getEff(dataB,0.97)
n_overT_0p98_B  = getEff(dataB,0.98)
n_overT_0p99_B  = getEff(dataB,0.99)
n_overT_0p995_B = getEff(dataB,0.995)
n_overT_0p999_B = getEff(dataB,0.999)

# Purity Numerator: denominator + highest score > Threshold + predicted true
n_matched_overT_0p1_S   = getPurity(dataS,0.1)
n_matched_overT_0p3_S   = getPurity(dataS,0.3)
n_matched_overT_0p5_S   = getPurity(dataS,0.5)
n_matched_overT_0p7_S   = getPurity(dataS,0.7)
n_matched_overT_0p8_S   = getPurity(dataS,0.8)
n_matched_overT_0p9_S   = getPurity(dataS,0.9)
n_matched_overT_0p95_S  = getPurity(dataS,0.95)
n_matched_overT_0p96_S  = getPurity(dataS,0.96)
n_matched_overT_0p97_S  = getPurity(dataS,0.97)
n_matched_overT_0p98_S  = getPurity(dataS,0.98)
n_matched_overT_0p99_S  = getPurity(dataS,0.99)
n_matched_overT_0p995_S = getPurity(dataS,0.995)
n_matched_overT_0p999_S = getPurity(dataS,0.999)

# Print efficiencies
printing_string_S = '''- Training: {} -
Signal:
 - Total events (denominator) : {:5}
 - Efficiencies / Purities
     - eff T > 0.1   : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.3   : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.5   : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.7   : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.8   : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.9   : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.95  : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.96  : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.97  : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.98  : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.99  : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.995 : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
     - eff T > 0.999 : {:5} ({:3.2f}%)  /  purity: {:5} ({:3.2f}%)
'''.format(
    versionName     ,
    tot_events_S    ,
    n_overT_0p1_S   , (100. * n_overT_0p1_S   / tot_events_S), n_matched_overT_0p1_S   , (100. * n_matched_overT_0p1_S   / n_overT_0p1_S  ),
    n_overT_0p3_S   , (100. * n_overT_0p3_S   / tot_events_S), n_matched_overT_0p3_S   , (100. * n_matched_overT_0p3_S   / n_overT_0p3_S  ),
    n_overT_0p5_S   , (100. * n_overT_0p5_S   / tot_events_S), n_matched_overT_0p5_S   , (100. * n_matched_overT_0p5_S   / n_overT_0p5_S  ),
    n_overT_0p7_S   , (100. * n_overT_0p7_S   / tot_events_S), n_matched_overT_0p7_S   , (100. * n_matched_overT_0p7_S   / n_overT_0p7_S  ),
    n_overT_0p8_S   , (100. * n_overT_0p8_S   / tot_events_S), n_matched_overT_0p8_S   , (100. * n_matched_overT_0p8_S   / n_overT_0p8_S  ),
    n_overT_0p9_S   , (100. * n_overT_0p9_S   / tot_events_S), n_matched_overT_0p9_S   , (100. * n_matched_overT_0p9_S   / n_overT_0p9_S  ),
    n_overT_0p95_S  , (100. * n_overT_0p95_S  / tot_events_S), n_matched_overT_0p95_S  , (100. * n_matched_overT_0p95_S  / n_overT_0p95_S ),
    n_overT_0p96_S  , (100. * n_overT_0p96_S  / tot_events_S), n_matched_overT_0p96_S  , (100. * n_matched_overT_0p96_S  / n_overT_0p96_S ),
    n_overT_0p97_S  , (100. * n_overT_0p97_S  / tot_events_S), n_matched_overT_0p97_S  , (100. * n_matched_overT_0p97_S  / n_overT_0p97_S ),
    n_overT_0p98_S  , (100. * n_overT_0p98_S  / tot_events_S), n_matched_overT_0p98_S  , (100. * n_matched_overT_0p98_S  / n_overT_0p98_S ),
    n_overT_0p99_S  , (100. * n_overT_0p99_S  / tot_events_S), n_matched_overT_0p99_S  , (100. * n_matched_overT_0p99_S  / n_overT_0p99_S ),
    n_overT_0p995_S , (100. * n_overT_0p995_S / tot_events_S), n_matched_overT_0p995_S , (100. * n_matched_overT_0p995_S / n_overT_0p995_S),
    n_overT_0p999_S , (100. * n_overT_0p999_S / tot_events_S), n_matched_overT_0p999_S , (100. * n_matched_overT_0p999_S / n_overT_0p999_S) if n_overT_0p999_S != 0 else 0.,
)
printing_string_B = '''
Background:
 - Original events   : {:5}
 - Total events      : {:5} ({:3.2f}%)
 - Efficiencies
     - eff T > 0.1   : {:5} ({:3.2f}%)
     - eff T > 0.3   : {:5} ({:3.2f}%)
     - eff T > 0.5   : {:5} ({:3.2f}%)
     - eff T > 0.7   : {:5} ({:3.2f}%)
     - eff T > 0.8   : {:5} ({:3.2f}%)
     - eff T > 0.9   : {:5} ({:3.2f}%)
     - eff T > 0.95  : {:5} ({:3.2f}%)
     - eff T > 0.96  : {:5} ({:3.2f}%)
     - eff T > 0.97  : {:5} ({:3.2f}%)
     - eff T > 0.98  : {:5} ({:3.2f}%)
     - eff T > 0.99  : {:5} ({:3.2f}%)
     - eff T > 0.995 : {:5} ({:3.2f}%)
     - eff T > 0.999 : {:5} ({:3.2f}%)
'''.format(
    original_events_B      , 
    tot_events_B           , (100. * tot_events_B    / original_events_B),
    n_overT_0p1_B          , (100. * n_overT_0p1_B   / original_events_B),
    n_overT_0p3_B          , (100. * n_overT_0p3_B   / original_events_B),
    n_overT_0p5_B          , (100. * n_overT_0p5_B   / original_events_B),
    n_overT_0p7_B          , (100. * n_overT_0p7_B   / original_events_B),
    n_overT_0p8_B          , (100. * n_overT_0p8_B   / original_events_B),
    n_overT_0p9_B          , (100. * n_overT_0p9_B   / original_events_B),
    n_overT_0p95_B         , (100. * n_overT_0p95_B  / original_events_B),
    n_overT_0p96_B         , (100. * n_overT_0p96_B  / original_events_B),
    n_overT_0p97_B         , (100. * n_overT_0p97_B  / original_events_B),
    n_overT_0p98_B         , (100. * n_overT_0p98_B  / original_events_B),
    n_overT_0p99_B         , (100. * n_overT_0p99_B  / original_events_B),
    n_overT_0p995_B        , (100. * n_overT_0p995_B / original_events_B),
    n_overT_0p999_B        , (100. * n_overT_0p999_B / original_events_B),
)
print(printing_string_S + printing_string_B)

#import pdb; pdb.set_trace()


'''
w/3 matched --> branch "genmatched"
score > T   --> branch "highest_score"
GenMatch    --> branch "predicted_true"
'''