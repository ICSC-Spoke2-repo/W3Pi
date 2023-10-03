# General imports
import pdb
import pandas as pd
import numpy as np
import math

# Define events to be used for training/validation/test (i.e. fill appropriate columns)
def train_test_valid_split(dframe, train_size, valid_size):
  dframe = dframe.sample(frac=1, random_state=2023).reset_index(drop=True)
  ti = math.ceil(len(dframe.index)*train_size)
  vi = math.ceil(len(dframe.index)*valid_size)+ti
  dframe.loc[  :ti-1, 'is_train'] = True
  dframe.loc[ti:vi-1, 'is_valid'] = True
  dframe.loc[vi:    , 'is_test' ] = True
  return dframe


# Make output pandas DF
def make_PandasDF (sframe_numpyzed, bframe_numpyzed):

  # Merge signal and background numpyzed dictionaries into one
  merged_dict = merge_numpyzed_dictionaries(sframe_numpyzed, bframe_numpyzed)

  # Make Pandas DF from the merged dictionary
  oframe = pd.DataFrame.from_dict(merged_dict)

  return oframe


# Merge two dictionaries with same keys
def merge_numpyzed_dictionaries (sframe_numpyzed, bframe_numpyzed):

  # Make sure dicts have the same keys
  assert ( sframe_numpyzed.keys() == bframe_numpyzed.keys() ), "sframe_numpyzed and bframe_numpyzed have different keys!!"

  # Merge dictionaries
  dicts = [sframe_numpyzed, bframe_numpyzed]
  ret = {}
  for k in sframe_numpyzed:
    ret[k] = np.concatenate(list(ret[k] for ret in dicts))

  return ret


def recast_PandasDF (oframe, OUT_BRANCHES):

  for k, (_, t) in OUT_BRANCHES.items():
    oframe[k] = oframe[k].astype(t) if not t is None else oframe[k]

  return oframe
