// ------------------------------------------------
// General includes
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numeric>
#include <vector>

// ------------------------------------------------
// General defines
#define DEBUG false
#define MW_PDG 80.377 //# m_W from PDG 2022

// ------------------------------------------------
// Define ROOT aliases
using cRVecF = const ROOT::VecOps::RVec<Float_t>&;
using cRVecI = const ROOT::VecOps::RVec<Int_t>&;
using tlv = ROOT::Math::PtEtaPhiMVector;

// ------------------------------------------------
// Triplet defines
struct triplet_idx {
  int idx0;
  int idx1;
  int idx2;
};

triplet_idx make_triplet_idx(int i, int j, int k) {
    triplet_idx mytriplet = {i, j, k};
    return mytriplet;
}


// ------------------------------------------------
// Check if event is gen-matched
bool add_genmatched (cRVecI L1Puppi_GenPiIdx)
{
    bool found0 = (std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 0) != L1Puppi_GenPiIdx.end());
    bool found1 = (std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 1) != L1Puppi_GenPiIdx.end());
    bool found2 = (std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 2) != L1Puppi_GenPiIdx.end());

    if (found0 && found1 && found2)
        return true;
    else
        return false;
}


// ------------------------------------------------
// Return the list of index of the gen-matched L1Puppi candidates (for Signal ntuples)
std::vector<int> add_candidate_matched_idxs (cRVecI L1Puppi_GenPiIdx)
{
    int idx0 = std::distance(std::begin(L1Puppi_GenPiIdx), std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 0));
    int idx1 = std::distance(std::begin(L1Puppi_GenPiIdx), std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 1));
    int idx2 = std::distance(std::begin(L1Puppi_GenPiIdx), std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 2));

    std::vector<int> reco_idxs {idx0, idx1, idx2};

    return reco_idxs;
}


// ------------------------------------------------
// Check if the pdgId is good (pdgId == +211 || -211 || 11 || -11) or not
bool good_pdg_id (Int_t id)
{
    bool ret = ( std::abs(id) == 211 || std::abs(id) == 11 ) ? true : false;
    return ret;
}


// ------------------------------------------------
// Return the final indexs of triplet:
// - Takes as input the list of candidate idxs of reco particles in the event
//    - For signal it's already the matched triplet (only 3 idxs)
//    - For background is the full list of idxs
// - Selection of the final triplet
//    - Skim the candidate idxs list (PDG ID + charge selection)
//    - Make list of combinations of triplet indexes
//    - Skim the list of triplets to keep only the ones with |sum(charges)| == 1
//      - Possibly add other selections to skim the triplets list
//    - If size of the triplets list is >= 1:
//      - Take random triplet from the list 
//    - Else:
//      - no good triplet --> skip event, i.e. return (-1,-1,-1)
// This procedure is the same for both signal and background events
std::vector<int> add_final_triplet_idxs (std::vector<int> candidate_idxs, cRVecI L1Puppi_pdgId, cRVecI L1Puppi_charge)
{
    // Declare output vector
    std::vector<int> final_idxs;

    // Make list of indexes filtered on good PDG ID and with charge +/-1
    std::vector<int> good_indexs;
    for (auto idx : candidate_idxs)
        if (good_pdg_id(L1Puppi_pdgId[idx]) && std::abs(L1Puppi_charge[idx]) == 1)
            good_indexs.push_back(idx);

    if (good_indexs.size() < 3) /* No triplet possible - return a fake index */
    {
        final_idxs.push_back(-1.);
    }
    else /* Make and select final triplet */
    {
        //std::cout << "- good_indexs: ";
        //for (auto el: good_indexs) std::cout << el << ", ";
        //std::cout << std::endl;

        // Make list of combinations from good_indexs
        std::vector<triplet_idx> triplet_list;
        for (auto i = 0; i < good_indexs.size()-2; i++)
            for (auto j = i+1; j < good_indexs.size()-1; j++)
                for (auto k = j+1; k < good_indexs.size(); k++)
                    triplet_list.push_back(make_triplet_idx(good_indexs[i],good_indexs[j],good_indexs[k]));

        //std::cout << "triplet_list: ";
        //for (auto el: triplet_list) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
        //std::cout << std::endl;

        // Skim triplet_list to only keep the ones with |sum(charges)| == 1
        std::vector<triplet_idx> final_triplets;
        for (auto triplet : triplet_list)
            if ( std::abs(L1Puppi_charge[triplet.idx0] + L1Puppi_charge[triplet.idx1] + L1Puppi_charge[triplet.idx2]) == 1 )
                final_triplets.push_back(triplet);
        // FIXME: ... possibly add more selections on triplets here ...

        //std::cout << "final_triplets: ";
        //for (auto el: final_triplets) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
        //std::cout << std::endl;

        // Now select final triplet to be returned
        // Using seed '0' for TRandom3 guarantees different random numbers at each event, see:
        // https://root.cern.ch/doc/master/classTRandom3.html#aa0f90fdd325edead6b80afcb05099348
        TRandom3 rand(0);
        if (final_triplets.size() > 1) /* Select a random triplet */
        {
            // Get random index from final_triplets
            int j = rand.Integer(final_triplets.size());

            // Return it
            final_idxs.push_back(final_triplets[j].idx0);
            final_idxs.push_back(final_triplets[j].idx1);
            final_idxs.push_back(final_triplets[j].idx2);
        }
        else if (final_triplets.size() == 1) /* Return the only one */
        {
            final_idxs.push_back(final_triplets[0].idx0);
            final_idxs.push_back(final_triplets[0].idx1);
            final_idxs.push_back(final_triplets[0].idx2);
        }
        else /* No good triplet - return a fake index */
        {
            final_idxs.push_back(-1.);
        }
    }

    //std::cout << "final_idxs: ";
    //for (auto el: final_idxs) std::cout << el << ", ";
    //std::cout << std::endl;

    // Shuffle idxs before returning them
    std::random_shuffle(final_idxs.begin(), final_idxs.end());

    return final_idxs;
}


// ------------------------------------------------
// Same as add_final_triplet_idxs, but build triplet from pivot, i.e. using highest pT pion as seed
std::vector<int> add_final_triplet_idxs_from_pivot (std::vector<int> candidate_idxs, cRVecI L1Puppi_pdgId, cRVecI L1Puppi_charge, cRVecF L1Puppi_pt)
{
    // Declare output vector
    std::vector<int> final_idxs;

    // Make list of indexes filtered on good PDG ID and with charge +/-1
    std::vector<int> good_indexs;
    for (auto idx : candidate_idxs)
        if (good_pdg_id(L1Puppi_pdgId[idx]) && std::abs(L1Puppi_charge[idx]) == 1)
            good_indexs.push_back(idx);

    if (good_indexs.size() < 3) /* No triplet possible - return a fake index */
    {
        final_idxs.push_back(-1.);
    }
    else /* Make and select final triplet */
    {

        //std::cout << "- good_indexs: ";
        //for (int i=0; i<good_indexs.size(); i++) std::cout << good_indexs[i] << "(" << L1Puppi_pt[good_indexs[i]] << "), ";
        //std::cout << std::endl;

        // Find pivot index
        int pivot_idx = -1.;
        float max_pt = 0.;
        for (auto idx : good_indexs)
        {
            if (L1Puppi_pt[idx] > max_pt)
            {
                max_pt = L1Puppi_pt[idx];
                pivot_idx = idx;
            }
        }

        //std::cout << " pivot idx: " << pivot_idx << " with pt = " << max_pt << std::endl;

        // Make list of triplets from good_indexs as: pivot_idx + doublet
        std::vector<triplet_idx> triplet_list;
        for (auto i = 0; i < good_indexs.size()-1; i++)
            for (auto j = i+1; j < good_indexs.size(); j++)
                if ( good_indexs[i] != pivot_idx && good_indexs[j] != pivot_idx)
                    triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[i], good_indexs[j]));

        //std::cout << " triplet_list: ";
        //for (auto el: triplet_list) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
        //std::cout << std::endl;

        // Skim triplet_list to only keep the ones with |sum(charges)| == 1
        std::vector<triplet_idx> final_triplets;
        for (auto triplet : triplet_list)
            if ( std::abs(L1Puppi_charge[triplet.idx0] + L1Puppi_charge[triplet.idx1] + L1Puppi_charge[triplet.idx2]) == 1 )
                final_triplets.push_back(triplet);
        // FIXME: ... possibly add more selections on triplets here ...

        //std::cout << " final_triplets: ";
        //for (auto el: final_triplets) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
        //std::cout << std::endl;

        // Now select final triplet to be returned
        // Using seed '0' for TRandom3 guarantees different random numbers at each event, see:
        // https://root.cern.ch/doc/master/classTRandom3.html#aa0f90fdd325edead6b80afcb05099348
        TRandom3 rand(0);
        if (final_triplets.size() > 1) /* Select a random triplet */
        {
            // Get random index from final_triplets
            int j = rand.Integer(final_triplets.size());

            // Return it
            final_idxs.push_back(final_triplets[j].idx0);
            final_idxs.push_back(final_triplets[j].idx1);
            final_idxs.push_back(final_triplets[j].idx2);
        }
        else if (final_triplets.size() == 1) /* Return the only one */
        {
            final_idxs.push_back(final_triplets[0].idx0);
            final_idxs.push_back(final_triplets[0].idx1);
            final_idxs.push_back(final_triplets[0].idx2);
        }
        else /* No good triplet - return a fake index */
        {
            final_idxs.push_back(-1.);
        }
    }

    //std::cout << " final_idxs: ";
    //for (auto el: final_idxs) std::cout << el << ", ";
    //std::cout << std::endl;

    // Shuffle idxs before returning them
    std::random_shuffle(final_idxs.begin(), final_idxs.end());

    return final_idxs;
}


// ------------------------------------------------
// Add flag which is True only for about "max_entries" random events
bool add_evt_to_keep_flag(float frac_to_keep)
{
    TRandom3 rand(0);
    float rand_n = rand.Uniform(0,1);
    if (rand_n < frac_to_keep)
        return true;
    else
        return false;
}

