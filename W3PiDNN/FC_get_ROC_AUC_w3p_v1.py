# References:
# - https://www.dlology.com/blog/simple-guide-on-how-to-generate-roc-plot-for-keras-classifier/

# General imports
import os
import sys ; sys.path.append(os.getcwd())
import argparse

# ROOT and utils imports
import ROOT
import pandas as pd
import matplotlib.pyplot as plt

# Keras and TF imports
import tensorflow as tf
from sklearn.metrics import roc_curve, auc

# Custom imports
from config.setup_v1 import FEATURES

# ----- Parse arguments -----
parser = argparse.ArgumentParser('Simple Script for the w3pDNN Evaluation.\n\
Pass different arguments in the form --some-setup arg1='"string_val"' args2=float_val (note the multiple quotes for strings)'
)
parser.add_argument('-i', '--input', required=True, help='input h5 file name' )

args = parser.parse_args()

'''
python3 FC_get_ROC_AUC_w3p_v1.py \
  -i hdf_nL1Puppi_11_pivot_pt_ordered_15_4_3_50_110_acceptance_iso
'''

# ----- Load data -----
data = pd.read_hdf('hdf_files/'+args.input+'.h5')

# Select 'is_test' events only
data_test  = data[data['is_test']]

# Define inputs and labels
test_inputs  = data_test[ [f for f in FEATURES] ]
test_labels  = data_test['class']

# ----- Models -----
models = ['20', '21__35_35_25', '21__35_20_20_25_35']
predictions = []
fpr_values  = []
tpr_values  = []
auc_values  = []

for model in models:
  my_model = tf.keras.models.load_model('trainings/w3pDNN_v'+model)
  y_predicted = my_model.predict(test_inputs).ravel()
  fpr_keras, tpr_keras, _ = roc_curve(test_labels, y_predicted)
  auc_keras = auc(fpr_keras, tpr_keras)

  predictions.append( y_predicted )
  fpr_values .append( fpr_keras   )
  tpr_values .append( tpr_keras   )
  auc_values .append( auc_keras   )


fig = plt.figure(figsize=(10, 7), dpi=100) 
plt.plot([0, 1], [0, 1], 'k--')
for i,model in enumerate(models):
  plt.plot(
    fpr_values[i],
    tpr_values[i],
    label='w3pDNN_v'+model+' (area = {:.5f})'.format(auc_values[i])
  )
plt.xlabel('False positive rate')
plt.ylabel('True positive rate')
plt.title('ROC curve')
plt.legend(loc='best')
plt.savefig('plots/AUC_ROC.png')
plt.close(fig)

#import pdb; pdb.set_trace()

'''
# Zoom in view of the upper left corner.
plt.figure(2)
plt.xlim(0, 0.2)
plt.ylim(0.8, 1)
plt.plot([0, 1], [0, 1], 'k--')
plt.plot(fpr_keras, tpr_keras, label='Keras (area = {:.3f})'.format(auc_keras))
plt.plot(fpr_rf, tpr_rf, label='RF (area = {:.3f})'.format(auc_rf))
plt.xlabel('False positive rate')
plt.ylabel('True positive rate')
plt.title('ROC curve (zoomed in at top left)')
plt.legend(loc='best')
plt.show()
'''

