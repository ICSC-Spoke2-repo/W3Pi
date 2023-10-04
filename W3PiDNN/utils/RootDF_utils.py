# General imports
import pdb
import ROOT

# Import c++ functions
ROOT.gInterpreter.ProcessLine('#include "utils/RootDF_utils.h"')

# ---------------------------------------------------------------------------------------------------
# Make output pandas DF
def make_numpy_from_RootDF (sframe, bframe, OUT_BRANCHES):

    # Prepare the signal dataframe
    signal_df = prepare_signal_df(sframe, OUT_BRANCHES)

    # Get filtered signal entries
    n_signal = signal_df.Count()

    # Make into NumPy dict
    signal_numpyzed = signal_df.AsNumpy(columns=OUT_BRANCHES.keys())
    print("---> Numpyzed signal!")

    # Prepare the background dataframe
    background_df = prepare_background_df(bframe, OUT_BRANCHES, 1.5 * n_signal.GetValue())

    # Make into NumPy dict
    background_numpyzed = background_df.AsNumpy(columns=OUT_BRANCHES.keys())
    print("---> Numpyzed background!")

    # Count calls of file-read
    s_calls = signal_df.GetNRuns()
    b_calls = background_df.GetNRuns()
    print("---> Calls for S:", s_calls, ' calls for B:', b_calls)

    return signal_numpyzed, background_numpyzed

# ---------------------------------------------------------------------------------------------------
### Signal Methods ###
# ---------------------------------------------------------------------------------------------------

# Prepare signal df starting from the original DF
# - filter on gen matched events only
# - add reco_idxs of the gen matched triplet
# - add new features for NN
def prepare_signal_df (df, OUT_BRANCHES):

    # Add gen_matched information and filter over it
    df = add_genmatched(df)
    df = df.Filter('genmatched')

    # Get list of candidate idxs
    # (in case of signal it's the idxs of the matched triplet)
    df = add_candidate_matched_idxs(df)

    # Seelct the final triplet candidate idxs (same selections for signal and background)
    df = add_final_triplet_idxs(df)

    # Filter only the events with a good triplet found
    df = df.Filter('reco_idxs.size() == 3')

    # Add TLorentzVectors of the three pions
    df = add_triplet_tlvs_from_idxs(df)

    # Now set all features needed for final df
    # 1. Add all the new columns
    df = add_reco_columns(df, OUT_BRANCHES)

    # 2. Set target column value (always 1 for signal)
    df = set_target_value(df, 1)

    return df


# Add genmatched information
def add_genmatched (df):
    df = df.Define('genmatched', 'add_genmatched(L1Puppi_GenPiIdx)')
    return df


# Add candidate indexes (idxs of the matched triplet for signal)
def add_candidate_matched_idxs (df):
    df = df.Define('candidate_idxs', 'add_candidate_matched_idxs(L1Puppi_GenPiIdx)')
    return df


# ---------------------------------------------------------------------------------------------------
### Background Methods ###
# ---------------------------------------------------------------------------------------------------

# Prepare background df starting from the original DF
def prepare_background_df (df, OUT_BRANCHES, max_entries):

    # Select only same number of entries as signal events
    df = add_evt_to_keep_flag(df, max_entries)
    df = df.Filter('evt_to_keep')

    # Get list of candidate idxs
    # (in case of background it's the full list of idxs of reco particles)
    df = add_candidate_reco_idxs(df)

    # Add random triplet idxs (using pivot)
    df = add_final_triplet_idxs(df, from_pivot = True)

    # Filter only the events with a good triplet found
    df = df.Filter('reco_idxs.size() == 3')

    # Add TLorentzVectors of the three pions
    df = add_triplet_tlvs_from_idxs(df)

    # Now set all features needed for final df
    # 1. Add all the new columns
    df = add_reco_columns(df, OUT_BRANCHES)

    # 2. Set target column value (always 0 for background)
    df = set_target_value(df, 0)

    return df


# Add a flag to be used to select only about "max_entries" out of the total entries
# NOTE: based on random selection --> i.e. works well when max_entries > 1000
def add_evt_to_keep_flag (df, max_entries):

    if max_entries < 0:
        # Keep all entries
        df = df.Define('evt_to_keep', 'true')
    else:
        # Compute total number of entries and the fraction to be kept
        tot_entries = df.Count().GetValue()
        frac_to_keep = 1. * max_entries / tot_entries
        df = df.Define('evt_to_keep', 'add_evt_to_keep_flag({})'.format(str(frac_to_keep)))

    return df


# Add candidate indexes (all reco idxs for background)
def add_candidate_reco_idxs (df):
    fill_reco_idxs = '''
    std::vector<int> v(nL1Puppi);
    std::iota(v.begin(), v.end(), 0);
    return v;
    '''
    df = df.Define('candidate_idxs', fill_reco_idxs)
    return df


# ---------------------------------------------------------------------------------------------------
### Common Methods ###
# ---------------------------------------------------------------------------------------------------

# Add final triplet idxs selected - same selections for signal and background
def add_final_triplet_idxs (df, from_pivot = False):
    if from_pivot:
        df = df.Define('reco_idxs', 'add_final_triplet_idxs_from_pivot(candidate_idxs, L1Puppi_pdgId, L1Puppi_charge, L1Puppi_pt)')
    else:
        df = df.Define('reco_idxs', 'add_final_triplet_idxs(candidate_idxs, L1Puppi_pdgId, L1Puppi_charge)')
    return df


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


# Add all the new columns from OUT_BRANCHES
def add_reco_columns (df, OUT_BRANCHES):
    for k, (v, _) in OUT_BRANCHES.items():
        df = df.Redefine(k, v) if k in df.GetColumnNames() else df.Define(k, v)
    return df


# Set target column value
# - 1  : signal
# - 0 : background
def set_target_value (df, sample_type):
    target_value = '1' if sample_type == 1 else '0'
    df = df.Redefine('class', target_value)
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
