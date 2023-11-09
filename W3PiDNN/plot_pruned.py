# References:
# - https://www.tensorflow.org/model_optimization/guide/pruning/pruning_with_sparsity_2_by_4

# General imports
import os
import sys ; sys.path.append(os.getcwd())
import argparse
import tempfile

# ROOT and utils imports
import ROOT
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Keras and TF imports
import tensorflow as tf
import tensorflow_model_optimization as tfmot


# ----- Parse arguments -----
parser = argparse.ArgumentParser('Simple Script for plotting the pruned NN.\n\
Pass different arguments in the form --some-setup arg1='"string_val"' args2=float_val (note the multiple quotes for strings)'
)
parser.add_argument('-v', '--NNversion'  , required=True, help='NN model version' )
parser.add_argument('-l', '--NNlabel'    , default=''   , help='additional label for NN version' )

args = parser.parse_args()

'''
python3 plot_pruned.py \
  -v 20 \
  -l p1
'''

# ----- Input TFLite model -----
input_tflite = 'prunings/w3pDNN_v'+args.NNversion+'_'+args.NNlabel+'/saved_tflite_model.tflite'
print('Loaded TFLite file:', input_tflite)

# ----- Auxiliary function to draw separation lines to see the structure clearly -----
def plot_separation_lines(height, width):

    block_size = [1, 1]

    # Add separation lines to the figure.
    num_hlines = int((height - 1) / block_size[0])
    num_vlines = int((width - 1) / block_size[1])
    line_y_pos = [y * block_size[0] for y in range(1, num_hlines + 1)]
    line_x_pos = [x * block_size[1] for x in range(1, num_vlines + 1)]

    for y_pos in line_y_pos:
        plt.plot([-0.5, width], [y_pos - 0.5 , y_pos - 0.5], color='w')

    for x_pos in line_x_pos:
        plt.plot([x_pos - 0.5, x_pos - 0.5], [-0.5, height], color='w')

# ----- TFLite Interpreter -----
# Load tflite file with the created pruned model
interpreter = tf.lite.Interpreter(model_path=input_tflite)
interpreter.allocate_tensors()

# Get all NN connections
details = interpreter.get_tensor_details()

# Pruned layers names
names = [
    'dense_1', 'dense_2', 'dense_3', 'dense_4',
    'dense_5', 'dense_6', 'dense_7', 'dense_8',
    'dense_9', 'dense_10', 'dense_11', 'dense_12',
    'dense_13', 'dense_14', 'dense_15'
]

# Plot layers
for name in names:

    # Layer name
    tensor_name = 'w3pDNN/'+name+'/MatMul'

    # Get data for the layer
    detail = [x for x in details if tensor_name in x["name"]]
    tensor_data = interpreter.tensor(detail[0]["index"])()
    print(f"Shape of {name} layer is {tensor_data.shape}")

    # Pruning visualization
    width = tensor_data.shape[1]
    height = tensor_data.shape[0]
    subset_values_to_display = tensor_data[0:height, 0:width]

    # Replace all non-zero values with ones
    val_ones = np.ones([height, width])
    val_zeros = np.zeros([height, width])
    subset_values_to_display = np.where(abs(subset_values_to_display) > 0, val_ones, val_zeros)

    # Visualize the weight tensor
    fig = plt.figure(figsize=(10, 7), dpi=100) 
    plot_separation_lines(height, width)

    # Plot and save
    plt.axis('off')
    plt.imshow(subset_values_to_display)
    plt.colorbar()
    plt.title("Pruned neurons for layer: "+name)
    #plt.show()
    plt.savefig('prunings/w3pDNN_v'+args.NNversion+'_'+args.NNlabel+'/pruned_'+name+'.png')
    plt.close(fig)



#import pdb; pdb.set_trace()


