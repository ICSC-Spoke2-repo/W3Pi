
import pandas as pd
import matplotlib.pyplot as plt
import os

from config.setup_v1 import FEATURES

# training version
#version = 'nL1Puppi_3_v4_pivot_shuffle_large'
version = 'nL1Puppi_3_pivot_pt_ordered_large'
version = 'nL1Puppi_4_pivot_pt_ordered_15_4_3_50_110'
version = 'nL1Puppi_4_pivot_pt_ordered_15_4_3_60_100'
version = 'nL1Puppi_5_pivot_pt_ordered_15_4_3_50_110'                 # with iso variables
version = 'nL1Puppi_6_pivot_pt_ordered_15_4_3_50_110_acceptance'      # add acceptance eta cuts
version = 'nL1Puppi_7_pivot_pt_ordered_15_4_3_50_110_acceptance'      # add vz composite vars and BIG bkg sample
version = 'nL1Puppi_8_pivot_pt_ordered_15_4_3_50_110_acceptance'      # add vz composite vars and back to small bkg sample
version = 'nL1Puppi_9_pivot_pt_ordered_15_4_3_50_110_acceptance'      # add to background the signal-non-matched events
version = 'nL1Puppi_10_pivot_pt_ordered_15_4_3_50_110_acceptance_iso' # same as 9 but add iso < 0.6 to selections
version = 'nL1Puppi_11_pivot_pt_ordered_15_4_3_50_110_acceptance_iso' # same as 10 but a bit larger number of bkg events (~ number of signal)

# Make output
if not os.path.isdir('hdf_files/plots_'+version):
    os.makedirs('hdf_files/plots_'+version)

# Read data
data = pd.read_hdf('hdf_files/hdf_'+version+'.h5')
#print(data)

s_data = data[data['class'] == 1]
b_data = data[data['class'] == 0]

# Tot events
n_t = data['run'].size
n_s = s_data['run'].size
n_b = b_data['run'].size

# signal train/test/validation
n_t_train = data[data['is_train'] == 1]['run'].size
n_t_test  = data[data['is_test'] == 1]['run'].size
n_t_valid = data[data['is_valid']  == 1]['run'].size

# signal train/test/validation
n_s_train = s_data[data['is_train'] == 1]['run'].size
n_s_test  = s_data[data['is_test'] == 1]['run'].size
n_s_valid = s_data[data['is_valid']  == 1]['run'].size

# signal train/test/validation
n_b_train = b_data[data['is_train'] == 1]['run'].size
n_b_test  = b_data[data['is_test'] == 1]['run'].size
n_b_valid = b_data[data['is_valid']  == 1]['run'].size

print('- Entries -')
print('total  : {:5}'.format(data  ['run'].size))
print('total s: {:5}'.format(s_data['run'].size))
print('total b: {:5}'.format(b_data['run'].size))

print('- Percentages of train/test/validation -')
print('Overall:')
print('  train      : {:.4} %'.format(100. * n_t_train / n_t) )
print('  test       : {:.4} %'.format(100. * n_t_test  / n_t) )
print('  validation : {:.4} %'.format(100. * n_t_valid / n_t) )
print('Signal:')
print('  train      : {:.4} %'.format(100. * n_s_train / n_s) )
print('  test       : {:.4} %'.format(100. * n_s_test  / n_s) )
print('  validation : {:.4} %'.format(100. * n_s_valid / n_s) )
print('Background:')
print('  train      : {:.4} %'.format(100. * n_b_train / n_b) )
print('  test       : {:.4} %'.format(100. * n_b_test  / n_b) )
print('  validation : {:.4} %'.format(100. * n_b_valid / n_b) )

# Plotting function
def make_plot (variable):
    fig = plt.figure(figsize=(10, 7), dpi=100) 
    # Plot histograms
    data[data['class'] == 1][variable].hist(alpha=0.6, grid=False, bins=30, density=True, label='signal'    , color='#3371ff')
    data[data['class'] == 0][variable].hist(alpha=0.6, grid=False, bins=30, density=True, label='background', color='#ffb833')

    # Customize plot
    plt.legend()
    plt.title(variable)
    plt.xlabel('Value')
    plt.ylabel('')

    # Save plot
    plt.savefig('hdf_files/plots_'+version+'/'+variable+'.png')
    plt.close(fig)

# Plotting
print('- Now plotting variables -')
for variable in FEATURES:
    make_plot(variable)

import pdb; pdb.set_trace()
