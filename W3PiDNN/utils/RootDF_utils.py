# General imports
import pdb
import ROOT
import numpy as np

# Import c++ functions
ROOT.gInterpreter.ProcessLine('#include "utils/RootDF_utils.h"')

# --------------------------------------------------------------------------------------------------------------------------------
# ### Make output pandas DF ###
# --------------------------------------------------------------------------------------------------------------------------------
# - Signal:
#     - add gen match
#     - filter genmatched
#  - Signal not-matched
#     - add gen matched
#     - filter on !genmatched
#     - add add_genmatched_1or2
#     - filter on add_genmatched_1or2
#  - Background
#     - add_evt_to_keep_flag
#     - Filter on evt_to_keep
def make_numpy_from_RootDF (sframe, bframe, OUT_BRANCHES, addSignalNonMatched):

    # Prepare signal DF:
    #  - add gen match info and filter on it
    signal_df = add_genmatched(sframe)
    signal_df = signal_df.Filter('genmatched')
    #  - prepare signal df for NN training
    signal_df = prepare_training_df(signal_df, OUT_BRANCHES, isSignal=True)
    #  - count signal events
    n_signal = signal_df.Count()
    #  - make into NumPy dict
    signal_numpyzed = signal_df.AsNumpy(columns=OUT_BRANCHES.keys())
    print("---> Numpyzed signal! Events:", n_signal.GetValue())

    # Prepare background DF
    #  - add evt_to_keep flag and filter on it
    background_df = add_evt_to_keep_flag(bframe, max_entries=n_signal.GetValue())
    background_df = background_df.Filter('evt_to_keep')
    #  - prepare background df for NN training
    background_df = prepare_training_df(background_df, OUT_BRANCHES, isSignal=False)
    #  - make into NumPy dict
    background_numpyzed = background_df.AsNumpy(columns=OUT_BRANCHES.keys())
    print("---> Numpyzed background! Events:", background_df.Count().GetValue())

    # Prepare signal-non-matched DF
    if addSignalNonMatched:
        #  - use only non matched signal events with only 1 or 2 matched pions
        nonmatched_df = add_genmatched(sframe)
        nonmatched_df = nonmatched_df.Filter('!genmatched')
        nonmatched_df = add_genmatched_1or2(nonmatched_df)
        nonmatched_df = nonmatched_df.Filter('genmatched_1or2')
        #  - prepare ignal-non-matched df for NN training
        nonmatched_df = prepare_training_df(nonmatched_df, OUT_BRANCHES, isSignal=False)
        #  - make into NumPy dict
        nonMatched_numpyzed = nonmatched_df.AsNumpy(columns=OUT_BRANCHES.keys())
        print("---> Numpyzed nonMatched! Events:", nonmatched_df.Count().GetValue())

        #  - merge with background numpy dictionary
        merge_list = [background_numpyzed, nonMatched_numpyzed]
        merged_bkg = {}
        for k in background_numpyzed.keys():
            merged_bkg[k] = np.concatenate(list(d[k] for d in merge_list))

        background_numpyzed  = merged_bkg
        print("---> Merged background and non-matched signal events!")

    # Count calls of file-read
    s_calls = signal_df.GetNRuns()
    b_calls = background_df.GetNRuns()
    print("---> Calls for S:", s_calls, ' calls for B:', b_calls)

    return signal_numpyzed, background_numpyzed


# ---------------------------------------------------------------------------------------------------
# Prepare DF for NN training
# ---------------------------------------------------------------------------------------------------
def prepare_training_df (df, OUT_BRANCHES, isSignal=False):

    # Add isolation of all L1Puppi objects immediately so it can be used for the selections
    df = add_isolation(df)

    # Add candidate indexes:
    # - Signal    : three matched pions
    # - Non-signal: full list of idxs of reco particles
    df = add_candidate_idxs(df, isSignal)

    # Seelct the final triplet candidate idxs
    df = add_final_triplet_idxs(df, from_pivot = True)

    # Filter only the events with a good triplet found
    df = df.Filter('reco_idxs.size() == 3')

    # Add TLorentzVectors of the three pions
    df = add_triplet_tlvs_from_idxs(df)

    # Now set all features needed for final df
    df = add_reco_columns(df, OUT_BRANCHES)

    # Set target column value { 1: signal , 0: background }
    if isSignal:
        df = set_target_value(df, '1')
    else:
        df = set_target_value(df, '0')

    return df


# ---------------------------------------------------------------------------------------------------
# Methods used in the above functions
# ---------------------------------------------------------------------------------------------------

# --------------------------------
# Add isolation without using TLVs
def add_isolation(df):
    df = df.Define('L1Puppi_iso', 'add_isolation(L1Puppi_pt, L1Puppi_eta, L1Puppi_phi)')
    return df

# --------------------------------
# Add candidate indexes:
def add_candidate_idxs(df, isSignal):
    if isSignal:
        df = df.Define('candidate_idxs', 'add_candidate_matched_idxs(L1Puppi_GenPiIdx)')
    else:
        fill_reco_idxs = '''
        std::vector<int> v(nL1Puppi);
        std::iota(v.begin(), v.end(), 0);
        return v;
        '''
        df = df.Define('candidate_idxs', fill_reco_idxs)
    return df

# --------------------------------
# Add candidate indexes (idxs of the matched triplet for signal)
def add_candidate_matched_idxs (df):
    df = df.Define('candidate_idxs', 'add_candidate_matched_idxs(L1Puppi_GenPiIdx)')
    return df

# --------------------------------
# Add final triplet idxs selected - same selections for signal and background
def add_final_triplet_idxs (df, from_pivot = False):
    if from_pivot:
        df = df.Define('reco_idxs', 'add_final_triplet_idxs_from_pivot(candidate_idxs, L1Puppi_pdgId, L1Puppi_charge, L1Puppi_pt, L1Puppi_eta, L1Puppi_phi, L1Puppi_mass, L1Puppi_iso)')
    else:
        df = df.Define('reco_idxs', 'add_final_triplet_idxs(candidate_idxs, L1Puppi_pdgId, L1Puppi_charge, L1Puppi_pt, L1Puppi_eta, L1Puppi_iso)')
    return df

# --------------------------------
# Add tlvs of triplet from idxs
def add_triplet_tlvs_from_idxs (df):
    makeTLV = '''
    int reco_idx = reco_idxs[{}];
    ROOT::Math::PtEtaPhiMVector tlv(L1Puppi_pt[reco_idx], L1Puppi_eta[reco_idx], L1Puppi_phi[reco_idx], L1Puppi_mass[reco_idx]);
    return tlv;
    '''
    df = df.Define('tlv0', makeTLV.format('0'))
    df = df.Define('tlv1', makeTLV.format('1'))
    df = df.Define('tlv2', makeTLV.format('2'))
    return df

# --------------------------------
# Add all the new columns from OUT_BRANCHES
def add_reco_columns (df, OUT_BRANCHES):
    for k, (v, _) in OUT_BRANCHES.items():
        df = df.Redefine(k, v) if k in df.GetColumnNames() else df.Define(k, v)
    return df

# --------------------------------
# Set target column value:
def set_target_value (df, target_value):
    df = df.Redefine('class', target_value)
    return df

# --------------------------------
# Add genmatched information: all 3 pions matched
def add_genmatched (df):
    df = df.Define('genmatched', 'add_genmatched(L1Puppi_GenPiIdx)')
    return df

# --------------------------------
# Add genmatched information: only 1 or 2 matched pions
def add_genmatched_1or2 (df):
    df = df.Define('genmatched_1or2', 'add_genmatched_1or2(L1Puppi_GenPiIdx)')
    return df

# --------------------------------
# Add a flag to be used to select only about "max_entries" out of the total entries
# NOTE: based on random selection --> i.e. works well when max_entries > 1000
def add_evt_to_keep_flag (df, max_entries):
    if max_entries < 0: # Keep all entries
        df = df.Define('evt_to_keep', 'true')
    else: # Compute total number of entries and the fraction to be kept
        tot_entries = df.Count().GetValue()
        triplet_sel_eff_with_iso = 0.009
        frac_to_keep = (1. * max_entries / tot_entries) / triplet_sel_eff_with_iso
        df = df.Define('evt_to_keep', 'add_evt_to_keep_flag({})'.format(str(frac_to_keep)))
    return df


# ---------------------------------------------------------------------------------------------------
### Inference Methods ###
# ---------------------------------------------------------------------------------------------------
# Prepare inference df
def prepare_inference_df (df, OUT_BRANCHES):

    # Add isolation of all L1Puppi objects immediately so it can be used for the selections
    df = add_isolation(df)

    # Get list of candidate idxs
    # (in case of inference it's the full list of idxs of reco particles)
    df = add_candidate_idxs(df, isSignal=False)

    # Add random triplet idxs (always using pivot)
    df = add_all_triplet_idxs(df)

    # Filter only the events with a good triplet found
    df = df.Filter('triplet_idxs.size() > 0 && triplet_idxs[0].idx0 >= 0')

    return df

# --------------------------------
# Add flag for gen-matched events within detector acceptance
def add_gen_acceptance(df):
    gen_acceptance = "(GenPi_pt[0]>2. && GenPi_pt[1]>2. && GenPi_pt[2]>2. && std::abs(GenPi_eta[0])<=2.4 && std::abs(GenPi_eta[1])<=2.4 && std::abs(GenPi_eta[2])<=2.4)"
    df = df.Define("gen_acceptance", gen_acceptance)
    return df

# --------------------------------
# Add list of all triplet idxs selected - same selections for signal and background
def add_all_triplet_idxs (df, from_pivot = False):
    df = df.Define('triplet_idxs', 'add_all_triplet_idxs_from_pivot(candidate_idxs, L1Puppi_pdgId, L1Puppi_charge, L1Puppi_pt, L1Puppi_eta, L1Puppi_phi, L1Puppi_mass, L1Puppi_iso)')
    return df


# ---------------------------------------------------------------------------------------------------
### General Methods ###
# ---------------------------------------------------------------------------------------------------

# Print current RDataFrame - works only with MultiThreading disabled
def print_dataframe (df, columns, nrows = 10):
  """
  Inputs:
   - (filtered) RDataFrame
  Output:
   - null (print to screen)

  ! WARNING ! only works with MultiThreading DISABLED!!
  """
  print('Printing the current RDF')

  # Printout
  d2 = df.Display(columns, nrows)
  print(d2.AsString())
