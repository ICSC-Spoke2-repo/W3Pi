import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

# training version

train_versions = [
    'v7', 'v8', 'v9', 'v10', 'v11', 'v11_v2', 'v12', 'v13',
    'v14', 'v15', 'v16', 'v17', 'v18', 'v19', 'v20',
    'v20_p1', 'v20_p2', 'v20_p3', 'v20_p4',
]
train_versions = [
    'v21__35_35_25'
]

for train_version in train_versions:

    version = 'w3pDNN_'+train_version+'_signalPU200_small'
    print("- Plotting version:", version)

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
    plt.savefig('NEW_highest_score_'+version+'.png')

    # Log y
    plt.yscale("log")
    plt.savefig('NEW_highest_score_'+version+'_log.png')

    plt.close(fig)

import pdb; pdb.set_trace()
