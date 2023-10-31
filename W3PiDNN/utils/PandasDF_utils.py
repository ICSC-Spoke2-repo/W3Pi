# General imports
import pdb
import pandas as pd
import numpy as np
import math
import ROOT

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


# Recast variable type for pandas and NN compression
def recast_PandasDF (oframe, OUT_BRANCHES):

  for k, (_, t) in OUT_BRANCHES.items():
    oframe[k] = oframe[k].astype(t) if not t is None else oframe[k]

  return oframe


# Get list of numpy arrays with inputs for the NN for each triplet in the event
def get_triplets_inputs(event):
    outs = []
    for triplet in event.triplet_idxs:
        i0 = triplet.idx0
        i1 = triplet.idx1
        i2 = triplet.idx2
        tlv0 = ROOT.Math.PtEtaPhiMVector(event.L1Puppi_pt[i0], event.L1Puppi_eta[i0], event.L1Puppi_phi[i0], event.L1Puppi_mass[i0])
        tlv1 = ROOT.Math.PtEtaPhiMVector(event.L1Puppi_pt[i1], event.L1Puppi_eta[i1], event.L1Puppi_phi[i1], event.L1Puppi_mass[i1])
        tlv2 = ROOT.Math.PtEtaPhiMVector(event.L1Puppi_pt[i2], event.L1Puppi_eta[i2], event.L1Puppi_phi[i2], event.L1Puppi_mass[i2])

        nn_inputs = np.array([
            event.L1Puppi_pt[i0],
            event.L1Puppi_pt[i1],
            event.L1Puppi_pt[i2],
#            event.L1Puppi_eta[i0],
#            event.L1Puppi_eta[i1],
#            event.L1Puppi_eta[i2],
#            event.L1Puppi_phi[i0],
#            event.L1Puppi_phi[i1],
#            event.L1Puppi_phi[i2],
#            event.L1Puppi_mass[i0],
#            event.L1Puppi_mass[i1],
#            event.L1Puppi_mass[i2],
#            event.L1Puppi_vz[i0],
#            event.L1Puppi_vz[i1],
#            event.L1Puppi_vz[i2],
#            event.L1Puppi_charge[i0],
#            event.L1Puppi_charge[i1],
#            event.L1Puppi_charge[i2],
            event.L1Puppi_pdgId[i0],
            event.L1Puppi_pdgId[i1],
            event.L1Puppi_pdgId[i2],
            event.L1Puppi_iso[i0],
            event.L1Puppi_iso[i1],
            event.L1Puppi_iso[i2],
#            event.L1Puppi_eta[i0] - event.L1Puppi_eta[i1],
#            event.L1Puppi_eta[i0] - event.L1Puppi_eta[i2],
#            event.L1Puppi_eta[i1] - event.L1Puppi_eta[i2],
#            event.L1Puppi_phi[i0] - event.L1Puppi_phi[i1],
#            event.L1Puppi_phi[i0] - event.L1Puppi_phi[i2],
#            event.L1Puppi_phi[i1] - event.L1Puppi_phi[i2],
            ROOT.Math.VectorUtil.DeltaR(tlv0, tlv1),
            ROOT.Math.VectorUtil.DeltaR(tlv0, tlv2),
            ROOT.Math.VectorUtil.DeltaR(tlv1, tlv2),
            (tlv0 + tlv1).M(),
            (tlv0 + tlv2).M(),
            (tlv1 + tlv2).M(),
            (tlv0 + tlv1).Pt(),
            (tlv0 + tlv2).Pt(),
            (tlv1 + tlv2).Pt(),
            event.L1Puppi_vz[i0] - event.L1Puppi_vz[i1],
            event.L1Puppi_vz[i0] - event.L1Puppi_vz[i2],
            event.L1Puppi_vz[i1] - event.L1Puppi_vz[i2],
#            (tlv0+tlv1+tlv2).M(),
            (tlv0+tlv1+tlv2).Pt(),
            max(ROOT.Math.VectorUtil.DeltaR(tlv0, tlv1), max(ROOT.Math.VectorUtil.DeltaR(tlv0, tlv2), ROOT.Math.VectorUtil.DeltaR(tlv1, tlv2))),
#            min(ROOT.Math.VectorUtil.DeltaR(tlv0, tlv1), min(ROOT.Math.VectorUtil.DeltaR(tlv0, tlv2), ROOT.Math.VectorUtil.DeltaR(tlv1, tlv2))),
            max( (event.L1Puppi_vz[i0]-event.L1Puppi_vz[i1]), max( (event.L1Puppi_vz[i0]-event.L1Puppi_vz[i2]), (event.L1Puppi_vz[i1]-event.L1Puppi_vz[i2]) ) ),
        ])

        outs.append(nn_inputs)

    return outs
