import numpy as np; np.object = object;

from tensorflow        import keras
from keras.constraints import max_norm
from keras.optimizers  import Adam

# Branches needed in the input trees - otherwise program will fail using assert
NEEDED_BRANCHES = [
  'run'             ,
  'luminosityBlock' ,
  'event'           ,
# 'bunchCrossing'   , # not present in v1 ntuples
  'nL1Puppi'        ,
  'L1Puppi_eta'     ,
  'L1Puppi_mass'    ,
  'L1Puppi_phi'     ,
  'L1Puppi_pt'      ,
  'L1Puppi_vz'      ,
  'L1Puppi_charge'  ,
  'L1Puppi_pdgId'   ,
]

GEN_NEEDED_BRANCHES = [
  'L1Puppi_GenPiIdx' ,
]

# Output branches in the HD5 file (for the NN training)
OUT_BRANCHES = {
  # Event variables
  'run'            : ('run'             , 'int32'),
  'lumi'           : ('luminosityBlock' , 'int32'),
  'event'          : ('event'           , 'int32'),
# 'bunchCrossing'  : ('bunchCrossing'   , 'int64'), # not present in v1 ntuples
  'nL1Puppi'       : ('nL1Puppi'        , 'int32'),

  # Pions variables
  'pi0_pt'         : ('L1Puppi_pt[reco_idxs[0]]'    , 'float32'),
  'pi1_pt'         : ('L1Puppi_pt[reco_idxs[1]]'    , 'float32'),
  'pi2_pt'         : ('L1Puppi_pt[reco_idxs[2]]'    , 'float32'),

  'pi0_eta'        : ('L1Puppi_eta[reco_idxs[0]]'   , 'float32'),
  'pi1_eta'        : ('L1Puppi_eta[reco_idxs[1]]'   , 'float32'),
  'pi2_eta'        : ('L1Puppi_eta[reco_idxs[2]]'   , 'float32'),

  'pi0_phi'        : ('L1Puppi_phi[reco_idxs[0]]'   , 'float32'),
  'pi1_phi'        : ('L1Puppi_phi[reco_idxs[1]]'   , 'float32'),
  'pi2_phi'        : ('L1Puppi_phi[reco_idxs[2]]'   , 'float32'),

  'pi0_mass'       : ('L1Puppi_mass[reco_idxs[0]]'  , 'float32'), # Always set to:
  'pi1_mass'       : ('L1Puppi_mass[reco_idxs[1]]'  , 'float32'), # - 0.129883 for pions
  'pi2_mass'       : ('L1Puppi_mass[reco_idxs[2]]'  , 'float32'), # - 0.005005 for electrons

  'pi0_vz'         : ('L1Puppi_vz[reco_idxs[0]]'    , 'float32'),
  'pi1_vz'         : ('L1Puppi_vz[reco_idxs[1]]'    , 'float32'),
  'pi2_vz'         : ('L1Puppi_vz[reco_idxs[2]]'    , 'float32'),

  'pi0_charge'     : ('L1Puppi_charge[reco_idxs[0]]' , 'int32'),
  'pi1_charge'     : ('L1Puppi_charge[reco_idxs[1]]' , 'int32'),
  'pi2_charge'     : ('L1Puppi_charge[reco_idxs[2]]' , 'int32'),

  'pi0_pdgId'      : ('L1Puppi_pdgId[reco_idxs[0]]'  , 'int32'),
  'pi1_pdgId'      : ('L1Puppi_pdgId[reco_idxs[1]]'  , 'int32'),
  'pi2_pdgId'      : ('L1Puppi_pdgId[reco_idxs[2]]'  , 'int32'),

  # Pion pair variables (dR, dEta, dPhi)
  'dEta_01'        : ('L1Puppi_eta[reco_idxs[0]] - L1Puppi_eta[reco_idxs[1]]' , 'float32'),
  'dEta_02'        : ('L1Puppi_eta[reco_idxs[0]] - L1Puppi_eta[reco_idxs[2]]' , 'float32'),
  'dEta_12'        : ('L1Puppi_eta[reco_idxs[1]] - L1Puppi_eta[reco_idxs[2]]' , 'float32'),

  'dPhi_01'        : ('L1Puppi_phi[reco_idxs[0]] - L1Puppi_phi[reco_idxs[1]]' , 'float32'),
  'dPhi_02'        : ('L1Puppi_phi[reco_idxs[0]] - L1Puppi_phi[reco_idxs[2]]' , 'float32'),
  'dPhi_12'        : ('L1Puppi_phi[reco_idxs[1]] - L1Puppi_phi[reco_idxs[2]]' , 'float32'),

  'dR_01'          : ('ROOT::Math::VectorUtil::DeltaR(tlv0, tlv1)'            , 'float32'),
  'dR_02'          : ('ROOT::Math::VectorUtil::DeltaR(tlv0, tlv2)'            , 'float32'),
  'dR_12'          : ('ROOT::Math::VectorUtil::DeltaR(tlv1, tlv2)'            , 'float32'),

  'm_01'           : ('(tlv0 + tlv1).M()'                                     , 'float32'),
  'm_02'           : ('(tlv0 + tlv2).M()'                                     , 'float32'),
  'm_12'           : ('(tlv1 + tlv2).M()'                                     , 'float32'),

  # Pion triplet variables (triplet invariant mass)
  'triplet_mass'   : ('(tlv0+tlv1+tlv2).M()'                                  , 'float32'),
  'triplet_pt'     : ('(tlv0+tlv1+tlv2).Pt()'                                 , 'float32'),
  'triplet_maxdR'  : ('max(dR_01, max(dR_02, dR_12))'                         , 'float32'),
  'triplet_mindR'  : ('min(dR_01, min(dR_02, dR_12))'                         , 'float32'),

  # NN Trining variables
  'is_train'       : ('false' , 'bool'),
  'is_valid'       : ('false' , 'bool'),
  'is_test'        : ('false' , 'bool'),

  # Target variable (True for signal, False for background)
  'class'          : ('0' , 'int16'),
}


# Features to be used as input in the NN
FEATURES = [
  b for b in OUT_BRANCHES.keys() if not b in [
    'is_train', 'is_valid', 'is_test', 'class',
    'run', 'lumi', 'event', 'bunchCrossing', 'nL1Puppi',
  ]
]


# NN Setup
SETUP = {
  'max_events': 250000 ,
  'target'    : 'class',
  'DENSE':    {
    'activation'        : 'relu'          ,
    'kernel_constraint' : max_norm(3)     ,
    'kernel_initializer': 'glorot_uniform',
  },
  'LAST':     {
    'activation': 'sigmoid',
  },
  'COMPILE':  {
    'loss'      : 'binary_crossentropy'   ,
    'optimizer' : Adam(learning_rate=1e-3),
    'metrics'   : 'accuracy'              ,
  },
  'FIT':      {
    'batch_size'          : 500 ,
    'epochs'              : 5   , #20
    'shuffle'             : True,
    'verbose'             : True,
    'use_multiprocessing' : True,
  },
}
