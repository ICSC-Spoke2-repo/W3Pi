# References:
# - https://www.tensorflow.org/model_optimization/guide/pruning/pruning_with_keras

# General imports
import os
import sys ; sys.path.append(os.getcwd())
import argparse
import zipfile
import tempfile

# ROOT and utils imports
import ROOT
import pandas as pd
import numpy as np

# Keras and TF imports
import tensorflow as tf
import tensorflow_model_optimization as tfmot
from keras.optimizers  import Adam

# Custom imports
from config.setup_v1 import FEATURES


# ----- Parse arguments -----
parser = argparse.ArgumentParser('Simple Script for the w3pDNN Evaluation.\n\
Pass different arguments in the form --some-setup arg1='"string_val"' args2=float_val (note the multiple quotes for strings)'
)
parser.add_argument('-v', '--NNversion'  , required=True, help='NN model version' )
parser.add_argument('-i', '--input'      , required=True, help='input h5 file name' )
parser.add_argument('-p', '--PrunVersion', required=True, help='Pruning version output' )
parser.add_argument('-l', '--NNlabel'    , default=''   , help='additional label for NN version' )

args = parser.parse_args()

'''
python3 FC_pruning_w3p_v1.py \
  -v 20 \
  -p 1 \
  -i hdf_nL1Puppi_11_pivot_pt_ordered_15_4_3_50_110_acceptance_iso
'''

# ----- Configurables -----
pruning_output = 'prunings/w3pDNN_v'+args.NNversion+args.NNlabel+'_p'+args.PrunVersion
batch_size     = 500
epochs         = 100

# ----- Load data -----
data = pd.read_hdf('hdf_files/'+args.input+'.h5')

# Select 'is_test' events only
data_test  = data[data['is_test']]
data_train = data[data['is_train']]
data_valid = data[data['is_valid']]

# Define model inputs and labels
test_inputs  = data_test[ [f for f in FEATURES] ]
test_labels  = data_test['class']
train_inputs = data_train[ [f for f in FEATURES] ]
train_labels = data_train['class']
valid_inputs = data_valid[ [f for f in FEATURES] ]
valid_labels = data_valid['class']


# ----- Load trained model -----
# Load model
my_model = tf.keras.models.load_model('trainings/w3pDNN_v'+args.NNversion+args.NNlabel)
my_model.summary()
keras_file = pruning_output + '/saved_keras_model.h5'
tf.keras.models.save_model(my_model, keras_file, include_optimizer=False)

# Baseline accuracy
baseline_model_loss, baseline_model_accuracy = my_model.evaluate(test_inputs, test_labels, verbose=0)


# ----- Pruning section -----
# Apply pruning to layers
def applyPruning2Dense(layer):
    if isinstance(layer, tf.keras.layers.Dense):
        #return tfmot.sparsity.keras.prune_low_magnitude(layer, **pruning_params)
        return tfmot.sparsity.keras.prune_low_magnitude(layer, **pruning_params_2)
    return layer

# Returns size of gzipped model, in bytes.
def get_gzipped_model_size(file):
  _, zipped_file = tempfile.mkstemp('.zip')
  with zipfile.ZipFile(zipped_file, 'w', compression=zipfile.ZIP_DEFLATED) as f:
    f.write(file)
  return os.path.getsize(zipped_file)

# Pruning parameters
end_step       = np.ceil(train_inputs.shape[0]/batch_size).astype(np.int32) * epochs
pruning_params = {
      'pruning_schedule': tfmot.sparsity.keras.PolynomialDecay(initial_sparsity=0.01,
                                                               final_sparsity=0.70,
                                                               begin_step=0,
                                                               end_step=end_step)
}
pruning_params_2 = {
      'pruning_schedule': tfmot.sparsity.keras.ConstantSparsity(target_sparsity=0.5,
                                                                begin_step=0,
                                                                frequency=100)
}

# Define pruned model
model_for_pruning = tf.keras.models.clone_model(my_model, clone_function=applyPruning2Dense)
model_for_pruning.compile(
    loss      = 'binary_crossentropy',
    optimizer = Adam(learning_rate=1e-3),
    metrics   = 'accuracy',
)
model_for_pruning.summary()

# Callbacks for pruning
callbacks = [
  tfmot.sparsity.keras.UpdatePruningStep(),
  tfmot.sparsity.keras.PruningSummaries(log_dir=pruning_output),
]
model_for_pruning.fit(train_inputs, train_labels,
                      batch_size=batch_size, epochs=epochs,
                      validation_data = [valid_inputs, valid_labels],
                      callbacks=callbacks)

# Pruned accuracy
model_for_pruning_loss, model_for_pruning_accuracy = model_for_pruning.evaluate(test_inputs, test_labels, verbose=0)


# ----- Pruning-strip section -----
# Save the pruning-stripped model
model_for_export = tfmot.sparsity.keras.strip_pruning(model_for_pruning)
model_for_export.compile(
    loss      = 'binary_crossentropy',
    optimizer = Adam(learning_rate=1e-3),
    metrics   = 'accuracy',
)
model_for_export.summary()
model_for_export.save(pruning_output)
pruned_keras_file = pruning_output + '/saved_pruned_model.h5'
tf.keras.models.save_model(model_for_export, pruned_keras_file, include_optimizer=False)


# ----- TFLite section -----
converter = tf.lite.TFLiteConverter.from_keras_model(model_for_export)
pruned_tflite_model = converter.convert()

pruned_tflite_file = pruning_output + '/saved_tflite_model.tflite'
with open(pruned_tflite_file, 'wb') as f:
  f.write(pruned_tflite_model)


# ----- Printouts -----

# Print accuracies
print('Baseline test accuracy:', baseline_model_accuracy, ' / Loss:', baseline_model_loss)
print('Pruned test accuracy  :', model_for_pruning_accuracy, ' / Loss:', model_for_pruning_loss)

# Print sizes
print("Size of gzipped baseline Keras model: %.2f bytes" % (get_gzipped_model_size(keras_file)))
print("Size of gzipped pruned Keras model  : %.2f bytes" % (get_gzipped_model_size(pruned_keras_file)))
print("Size of gzipped pruned TFlite model : %.2f bytes" % (get_gzipped_model_size(pruned_tflite_file)))

import pdb; pdb.set_trace()

