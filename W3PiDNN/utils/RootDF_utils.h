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

// -------------------------------------------------------------------------------------------
// ------------------------------------ Utility Functions ------------------------------------
// -------------------------------------------------------------------------------------------

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
// Check if event has at least 1 or 2 gen-matched pions
bool add_genmatched_1or2 (cRVecI L1Puppi_GenPiIdx)
{
    bool found0 = (std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 0) != L1Puppi_GenPiIdx.end());
    bool found1 = (std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 1) != L1Puppi_GenPiIdx.end());
    bool found2 = (std::find(L1Puppi_GenPiIdx.begin(), L1Puppi_GenPiIdx.end(), 2) != L1Puppi_GenPiIdx.end());

    if (found0 || found1 || found2)
        return true;
    else
        return false;
}

// ------------------------------------------------
// Check if the pdgId is good (pdgId == +211 || -211 || 11 || -11) or not
bool good_pdg_id (Int_t id)
{
    bool ret = ( std::abs(id) == 211 || std::abs(id) == 11 ) ? true : false;
    return ret;
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

// ------------------------------------------------
// Isolation methods
// Get dR between two particles as implemented in ROOT
float get_dR (float eta1, float phi1, float eta2, float phi2)
{
    float deta = eta2 - eta1;
    float dphi = phi2 - phi1;
    if ( dphi > M_PI )
    {
      dphi -= 2.0*M_PI;
    }
    else if ( dphi <= -M_PI )
    {
      dphi += 2.0*M_PI;
    }
    return std::sqrt(dphi*dphi + deta*deta);
}

// Add isolation defined without using TLVs
std::vector<float> add_isolation(cRVecF L1Puppi_pt, cRVecF L1Puppi_eta, cRVecF L1Puppi_phi)
{
    // Declare output
    std::vector<float> isolations;
    //std::cout << "---- MyIso ---" << std::endl;
    for (int i=0; i<L1Puppi_pt.size(); i++)
    {
        // Get sum pT for each particle
        float sum_pt = 0.;
        for (int j=0; j<L1Puppi_pt.size(); j++)
        {
            if (j == i)
                continue;
            else
            {
                float temp_dR = get_dR(L1Puppi_eta[i], L1Puppi_phi[i], L1Puppi_eta[j], L1Puppi_phi[j]);
                if (temp_dR > 0.01 && temp_dR < 0.25)
                    sum_pt += L1Puppi_pt[j];
                else
                    continue;
            }
        }
        isolations.push_back( sum_pt/L1Puppi_pt[i] );
    }

    return isolations;
}


// --------------------------------------------------------------------------------------------
// ------------------------------------ Selection/Triplets ------------------------------------
// --------------------------------------------------------------------------------------------

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
std::vector<int> add_final_triplet_idxs (std::vector<int> candidate_idxs, cRVecI L1Puppi_pdgId, cRVecI L1Puppi_charge, cRVecF L1Puppi_pt, cRVecF L1Puppi_eta, cRVecF L1Puppi_iso)
{
    // Declare output vector
    std::vector<int> final_idxs;

    // Make list of indexes filtered on good PDG ID and with charge +/-1
    std::vector<int> good_indexs;
    for (auto idx : candidate_idxs)
        if (good_pdg_id(L1Puppi_pdgId[idx]) && std::abs(L1Puppi_charge[idx]) == 1) // base selections
            if (L1Puppi_pt[idx] > 2. && std::abs(L1Puppi_eta[idx]) <= 2.4 )        // detector acceptance
                if (L1Puppi_iso[idx] <= 0.6)                                       // loose isolation cut
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
        // ... possibly add more selections on triplets here ...

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
std::vector<int> add_final_triplet_idxs_from_pivot (std::vector<int> candidate_idxs, cRVecI L1Puppi_pdgId, cRVecI L1Puppi_charge, cRVecF L1Puppi_pt, cRVecF L1Puppi_eta, cRVecF L1Puppi_phi, cRVecF L1Puppi_mass, cRVecF L1Puppi_iso)
{
    // Declare output vector
    std::vector<int> final_idxs;

    // Make list of indexes filtered on good PDG ID and with charge +/-1
    std::vector<int> good_indexs;
    for (auto idx : candidate_idxs)
        if (good_pdg_id(L1Puppi_pdgId[idx]) && std::abs(L1Puppi_charge[idx]) == 1) // base selections
            if (L1Puppi_pt[idx] > 2. && std::abs(L1Puppi_eta[idx]) <= 2.4 )        // detector acceptance
                if (L1Puppi_iso[idx] <= 0.6)                                       // loose isolation cut
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

        // Make list of triplets from good_indexs as: pivot_idx + doublet (ordered by pT)
        std::vector<triplet_idx> triplet_list;
        for (auto i = 0; i < good_indexs.size()-1; i++)
            for (auto j = i+1; j < good_indexs.size(); j++)
                if ( good_indexs[i] != pivot_idx && good_indexs[j] != pivot_idx)
                {
                    if ( L1Puppi_pt[good_indexs[i]] >= L1Puppi_pt[good_indexs[j]] )
                    {
                        triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[i], good_indexs[j]));
                    }
                    else
                    {
                        triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[j], good_indexs[i]));
                    }
                }

        //std::cout << " triplet_list: ";
        //for (auto el: triplet_list) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
        //std::cout << std::endl;

        // Skim triplet_list to only keep the ones with:
        std::vector<triplet_idx> final_triplets;
        for (auto triplet : triplet_list)
            // |sum(charges)| == 1
            if ( std::abs(L1Puppi_charge[triplet.idx0] + L1Puppi_charge[triplet.idx1] + L1Puppi_charge[triplet.idx2]) == 1 )
                // pt selection (15, 4, 3) || (15, 5, 5)
                if ( L1Puppi_pt[triplet.idx0] >= 15. && L1Puppi_pt[triplet.idx1] >= 4. && L1Puppi_pt[triplet.idx2] >= 3. )
                {
                    // Mass selection [50,110] || [60,100]
                    tlv tlv0(L1Puppi_pt[triplet.idx0], L1Puppi_eta[triplet.idx0], L1Puppi_phi[triplet.idx0], L1Puppi_mass[triplet.idx0]);
                    tlv tlv1(L1Puppi_pt[triplet.idx1], L1Puppi_eta[triplet.idx1], L1Puppi_phi[triplet.idx1], L1Puppi_mass[triplet.idx1]);
                    tlv tlv2(L1Puppi_pt[triplet.idx2], L1Puppi_eta[triplet.idx2], L1Puppi_phi[triplet.idx2], L1Puppi_mass[triplet.idx2]);
                    if ( (tlv0+tlv1+tlv2).M() >= 50. && (tlv0+tlv1+tlv2).M() <= 110. )
                        final_triplets.push_back(triplet);
                }
        // ... possibly add more selections on triplets here ...

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
    //std::random_shuffle(final_idxs.begin(), final_idxs.end()); // not needed anymore since we order the triplet idxs by pT

    return final_idxs;
}


// ------------------------------------------------
// Same as add_final_triplet_idxs, but build triplet from pivot, i.e. using highest pT pion as seed
std::vector<triplet_idx> add_all_triplet_idxs_from_pivot (std::vector<int> candidate_idxs, cRVecI L1Puppi_pdgId, cRVecI L1Puppi_charge, cRVecF L1Puppi_pt, cRVecF L1Puppi_eta, cRVecF L1Puppi_phi, cRVecF L1Puppi_mass, cRVecF L1Puppi_iso)
{
    // Declare output vector
    std::vector<triplet_idx> final_triplets;

    // Make list of indexes filtered on good PDG ID and with charge +/-1
    std::vector<int> good_indexs;
    for (auto idx : candidate_idxs)
        if (good_pdg_id(L1Puppi_pdgId[idx]) && std::abs(L1Puppi_charge[idx]) == 1) // base selections
            if (L1Puppi_pt[idx] > 2. && std::abs(L1Puppi_eta[idx]) <= 2.4 )        // detector acceptance
                if (L1Puppi_iso[idx] <= 0.6)                                       // loose isolation cut
                    good_indexs.push_back(idx);
    //std::cout << "- good_indexs: ";
    //for (int i=0; i<good_indexs.size(); i++) std::cout << good_indexs[i] << "(" << L1Puppi_pt[good_indexs[i]] << "), ";
    //std::cout << std::endl;

    if (good_indexs.size() < 3) /* No triplet possible - return a fake triplet */
    {
        final_triplets.push_back(make_triplet_idx(-1, -1, -1));
    }
    else /* Make and select all triplet with Pivot */
    {
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

        // Make list of triplets from good_indexs as: pivot_idx + doublet (ordered by pT)
        std::vector<triplet_idx> triplet_list;
        for (auto i = 0; i < good_indexs.size()-1; i++)
            for (auto j = i+1; j < good_indexs.size(); j++)
                if ( good_indexs[i] != pivot_idx && good_indexs[j] != pivot_idx)
                {
                    if ( L1Puppi_pt[good_indexs[i]] >= L1Puppi_pt[good_indexs[j]] )
                    {
                        triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[i], good_indexs[j]));
                    }
                    else
                    {
                        triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[j], good_indexs[i]));
                    }
                }
        //std::cout << " triplet_list: ";
        //for (auto el: triplet_list) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
        //std::cout << std::endl;

        // Skim triplet_list to only keep the ones with:
        for (auto triplet : triplet_list)
            // |sum(charges)| == 1
            if ( std::abs(L1Puppi_charge[triplet.idx0] + L1Puppi_charge[triplet.idx1] + L1Puppi_charge[triplet.idx2]) == 1 )
                // pt selection (15, 4, 3) || (15, 5, 5)
                if ( L1Puppi_pt[triplet.idx0] >= 15. && L1Puppi_pt[triplet.idx1] >= 4. && L1Puppi_pt[triplet.idx2] >= 3. )
                {
                    // Mass selection [50,110] || [60,100]
                    tlv tlv0(L1Puppi_pt[triplet.idx0], L1Puppi_eta[triplet.idx0], L1Puppi_phi[triplet.idx0], L1Puppi_mass[triplet.idx0]);
                    tlv tlv1(L1Puppi_pt[triplet.idx1], L1Puppi_eta[triplet.idx1], L1Puppi_phi[triplet.idx1], L1Puppi_mass[triplet.idx1]);
                    tlv tlv2(L1Puppi_pt[triplet.idx2], L1Puppi_eta[triplet.idx2], L1Puppi_phi[triplet.idx2], L1Puppi_mass[triplet.idx2]);
                    if ( (tlv0+tlv1+tlv2).M() >= 50. && (tlv0+tlv1+tlv2).M() <= 110. )
                        final_triplets.push_back(triplet);
                }

        // If no good triplet survives the skimming, return a fake one
        if (final_triplets.size() < 1)
        {
            final_triplets.push_back(make_triplet_idx(-1, -1, -1));
        }

    }

    //std::cout << " final_triplets: ";
    //for (auto el: final_triplets) std::cout << el.idx0 << "/" << el.idx1 << "/" << el.idx2 << ", ";
    //std::cout << std::endl;

    return final_triplets;
}


// ------------------------------------------------
// Add all triplets with Pietro's selections
// Prepare df with exact selections used by Pietro
//  - pdgId 211 || 11         OK
//  - pT 18, 15, 12           OK
//  - sum charge = 1          OK
//  - dR > 0.5 between pions
//  - 60 < m < 100            OK
//  - iso < 0.45              OK
std::vector<triplet_idx> add_pietro_triplet_idxs_from_pivot (std::vector<int> candidate_idxs, cRVecI L1Puppi_pdgId, cRVecI L1Puppi_charge, cRVecF L1Puppi_pt, cRVecF L1Puppi_eta, cRVecF L1Puppi_phi, cRVecF L1Puppi_mass, cRVecF L1Puppi_iso)
{
    // Declare output vector
    std::vector<triplet_idx> final_triplets;

    // Make list of indexes filtered on good PDG ID and with charge +/-1
    std::vector<int> good_indexs;
    for (auto idx : candidate_idxs)
        if (good_pdg_id(L1Puppi_pdgId[idx]))     // base selections
                if (L1Puppi_iso[idx] <= 0.45)    // isolation cut
                    good_indexs.push_back(idx);

    if (good_indexs.size() < 3) /* No triplet possible - return a fake triplet */
    {
        final_triplets.push_back(make_triplet_idx(-1, -1, -1));
    }
    else /* Make and select all triplet with Pivot */
    {
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

        // Make list of triplets from good_indexs as: pivot_idx + doublet (ordered by pT)
        std::vector<triplet_idx> triplet_list;
        for (auto i = 0; i < good_indexs.size()-1; i++)
            for (auto j = i+1; j < good_indexs.size(); j++)
                if ( good_indexs[i] != pivot_idx && good_indexs[j] != pivot_idx)
                {
                    if ( L1Puppi_pt[good_indexs[i]] >= L1Puppi_pt[good_indexs[j]] )
                    {
                        triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[i], good_indexs[j]));
                    }
                    else
                    {
                        triplet_list.push_back(make_triplet_idx(pivot_idx, good_indexs[j], good_indexs[i]));
                    }
                }

        // Skim triplet_list to only keep the ones with:
        for (auto triplet : triplet_list)
            // |sum(charges)| == 1
            if ( std::abs(L1Puppi_charge[triplet.idx0] + L1Puppi_charge[triplet.idx1] + L1Puppi_charge[triplet.idx2]) == 1 )
                // pt selection (18, 15, 12)
                if ( L1Puppi_pt[triplet.idx0] >= 18. && L1Puppi_pt[triplet.idx1] >= 15. && L1Puppi_pt[triplet.idx2] >= 12. )
                {
                    // dR > 0.5 between pions
                    float temp_dR01 = get_dR(L1Puppi_eta[triplet.idx0], L1Puppi_phi[triplet.idx0], L1Puppi_eta[triplet.idx1], L1Puppi_phi[triplet.idx1]);
                    float temp_dR02 = get_dR(L1Puppi_eta[triplet.idx0], L1Puppi_phi[triplet.idx0], L1Puppi_eta[triplet.idx2], L1Puppi_phi[triplet.idx2]);
                    float temp_dR12 = get_dR(L1Puppi_eta[triplet.idx1], L1Puppi_phi[triplet.idx1], L1Puppi_eta[triplet.idx2], L1Puppi_phi[triplet.idx2]);
                    if (temp_dR01 >= 0.5 && temp_dR02 >= 0.5 && temp_dR12 >= 0.5)
                    {
                        // Mass selection [60,100]
                        tlv tlv0(L1Puppi_pt[triplet.idx0], L1Puppi_eta[triplet.idx0], L1Puppi_phi[triplet.idx0], L1Puppi_mass[triplet.idx0]);
                        tlv tlv1(L1Puppi_pt[triplet.idx1], L1Puppi_eta[triplet.idx1], L1Puppi_phi[triplet.idx1], L1Puppi_mass[triplet.idx1]);
                        tlv tlv2(L1Puppi_pt[triplet.idx2], L1Puppi_eta[triplet.idx2], L1Puppi_phi[triplet.idx2], L1Puppi_mass[triplet.idx2]);
                        if ( (tlv0+tlv1+tlv2).M() >= 60. && (tlv0+tlv1+tlv2).M() <= 100. )
                            final_triplets.push_back(triplet);
                    }
                }

        // If no good triplet survives the skimming, return a fake one
        if (final_triplets.size() < 1)
        {
            final_triplets.push_back(make_triplet_idx(-1, -1, -1));
        }

    }

    return final_triplets;
}

// ------------------------------------------------
// Add final triplet candidate as the one with highest (pT x mW)
std::vector<int> add_pietro_final_triplet_idxs(std::vector<triplet_idx> triplet_idxs, cRVecF L1Puppi_pt, cRVecF L1Puppi_eta, cRVecF L1Puppi_phi, cRVecF L1Puppi_mass)
{
    // Declare output vector
    std::vector<int> final_idxs;

    // Loop on triplets to select the final one
    float best_pT_mW = -999.;
    int best_index = -1.;
    for (int i = 0; i < triplet_idxs.size(); i++)
    {
        // Get idxs and TLVs for each triplet
        int idx0 = triplet_idxs[i].idx0;
        int idx1 = triplet_idxs[i].idx1;
        int idx2 = triplet_idxs[i].idx2;
        tlv tlv0(L1Puppi_pt[idx0], L1Puppi_eta[idx0], L1Puppi_phi[idx0], L1Puppi_mass[idx0]);
        tlv tlv1(L1Puppi_pt[idx1], L1Puppi_eta[idx1], L1Puppi_phi[idx1], L1Puppi_mass[idx1]);
        tlv tlv2(L1Puppi_pt[idx2], L1Puppi_eta[idx2], L1Puppi_phi[idx2], L1Puppi_mass[idx2]);

        // Compute pT x mW for each triplet
        float temp_pT = (tlv0+tlv1+tlv2).Pt();
        float temp_mW = (tlv0+tlv1+tlv2).M();

        // Get the best triplet with highest (pT x mW)
        if ( temp_pT * temp_mW >= best_pT_mW)
        {
            best_pT_mW = temp_pT * temp_mW;
            best_index = i;
        }
    }

    // Return the best triplet
    final_idxs.push_back(triplet_idxs[best_index].idx0);
    final_idxs.push_back(triplet_idxs[best_index].idx1);
    final_idxs.push_back(triplet_idxs[best_index].idx2);
    return final_idxs;
}

