import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

# training version
version = 'w3pDNN_v6_signalPU200_small'

# Load data
data_s = pd.read_hdf('hdf_files/predicted/hdf_'+version+'.h5')
data_b = pd.read_hdf('hdf_files/predicted/hdf_'+version+'_background.h5')

# Figure
fig = plt.figure(figsize=(10, 7), dpi=100)

# Histograms
h_s = data_s['highest_score'].hist(alpha=0.6, grid=False, bins=100, density=True, label='signal'    , color='#3371ff')
h_b = data_b['highest_score'].hist(alpha=0.6, grid=False, bins=100, density=True, label='background', color='#ffb833')

# Customize plot
plt.legend()
plt.title('Triplet highest score')
plt.xlabel('Score')
plt.ylabel('')

# Save plot
plt.savefig('highest_score_'+version+'.png')

# Log y
plt.yscale("log") 
plt.savefig('highest_score_'+version+'_log.png')

plt.close(fig)

import pdb; pdb.set_trace()


'''
w/3 matched --> branch "genmatched"
score > T   --> branch "highest_score"
GenMatch    --> branch "predicted_true"


def make_plot (variable):
    fig = plt.figure(figsize=(10, 7), dpi=100) 
    # Plot histograms
    data[data['class'] == 1][variable].hist(alpha=0.6, grid=False, bins=30, label='signal'    , color='#3371ff')
    data[data['class'] == 0][variable].hist(alpha=0.6, grid=False, bins=30, label='background', color='#ffb833')

    # Customize plot
    plt.legend()
    plt.title(variable)
    plt.xlabel('Value')
    plt.ylabel('')

    # Save plot
    plt.savefig('hdf_files/plots_'+version+'/'+variable+'.png')
    plt.close(fig)


'''