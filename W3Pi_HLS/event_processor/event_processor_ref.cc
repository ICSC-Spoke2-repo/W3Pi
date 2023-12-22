#include "src/event_processor.h"

// Compute dR between two puppi objects
inline dr2_t deltaR2(const Puppi & p1, const Puppi & p2) {
    auto dphi = p1.hwPhi - p2.hwPhi;
    if (dphi > Puppi::INT_PI) dphi -= Puppi::INT_2PI;
    else if (dphi < -Puppi::INT_PI) dphi += Puppi::INT_2PI;
    auto deta = p1.hwEta - p2.hwEta;
    return dphi*dphi + deta*deta;
}

// Find highest pT puppi object
Puppi find_pivot_ref(unsigned int npuppi, const Puppi puppi[NPUPPI_MAX], const bool masked[NPUPPI_MAX]) {
    Puppi pivot;
    pivot.clear();
    for (int i = 0; i < npuppi; i++)
        if (!masked[i] && (pivot.hwPt < puppi[i].hwPt))
            pivot = puppi[i];
    return pivot;
}

// Find index highest pT puppi object
int find_pivot_idx_ref(unsigned int npuppi, const Puppi puppi[NPUPPI_MAX], const bool masked[NPUPPI_MAX]) {
    int iseed = -1;
    for (int i = 0; i < npuppi; ++i) {
      if (!masked[i] && (iseed == -1 || puppi[iseed].hwPt < puppi[i].hwPt)) {
        iseed = i;
      }
    }
    return iseed;
}

// Top function:
//  - filter candidates
//  - add isolation to filtered candidates
//  - update mask to consider only (iso_sum/pt) <= 0.6
//  - find pivot among them
void event_processor_ref (unsigned int npuppi, const Puppi input[NPUPPI_MAX], Puppi & pivot, Triplet triplets[NTRIPLETS_MAX], bool masked_triplets[NTRIPLETS_MAX])
{
    // Define masked lists to filter candidates
    bool masked[NPUPPI_MAX];

    // Filter candidates: loop and apply selections
    for (unsigned int i = 0; i < npuppi; i++)
    {
        masked[i] = ( (input[i].hwID <= 1 || input[i].hwID >= 6)                            ||
                      (input[i].hwPt <= 3)                                                  ||
                      (input[i].hwEta < -Puppi::ETA_CUT || input[i].hwEta > Puppi::ETA_CUT)
                    );
    }

    // Define and clean isolation array
    Puppi::pt_t output_absiso[npuppi];
    for (unsigned int j = 0; j < npuppi; ++j) output_absiso[j] = 0;

    // Define min/max isolation cones
    const dr2_t dr2_max = drToHwDr2(0.4), dr2_veto = drToHwDr2(0.1);

    // Compute isolation for all (filtered) candidates
    for (unsigned int j = 0; j < npuppi; ++j)
    {
        // Define each filtered candidate as seed to compute isolation
        Puppi seed = input[j];

        // Define list of pts to sum (particle inside isolation cone)
        Puppi::pt_t myiso = 0;

        // Loop on all particles to compute iso_sum
        for (unsigned int i = 0; i < npuppi; ++i) {
            // Get dR2
            dr2_t dr2 = deltaR2(seed, input[i]);
            // If inside and not in veto cone, get pt for iso computation
            myiso += (dr2 < dr2_max) && (dr2 > dr2_veto) ? input[i].hwPt : Puppi::pt_t(0);
        }

        // Store isolation value
        output_absiso[j] = masked[j] ? Puppi::pt_t(0) : myiso;
    }

    // Update mask to consider only (iso_sum/pt) <= 0.6
    for (unsigned int i = 0; i < npuppi; i++)
    {
        masked[i] = masked[i] ? masked[i] : (output_absiso[i]/input[i].hwPt) > 0.6;
    }

    // Find pivot (charged filtered candidate with highest pt)
    int pivot_idx = find_pivot_idx_ref(npuppi, input, masked);
    pivot = input[pivot_idx];

    // Debug printout
    //std::cout << "---> Ref Pivot idx: " << pivot_idx << std::endl;
    //std::cout << "     Ref Pivot pT : " << pivot.hwPt << " eta: " << pivot.hwEta*Puppi::ETAPHI_LSB << " pdgID: " << pivot.hwID << std::endl;

    // Build all triplets (pT ordered) starting from pivot
    int ntriplets = 0;
    for (unsigned int i = 0; i < npuppi-1; i++)
        for (unsigned int j = i+1; j < npuppi; j++)
        {
            if (ntriplets == NTRIPLETS_MAX)
                break;
            if ( i != pivot_idx && !masked[i] && j != pivot_idx && !masked[j])
            {
                if (input[i].hwPt >= input[j].hwPt)
                {
                    triplets[ntriplets] = Triplet(pivot_idx,i,j);
                }
                else
                {
                    triplets[ntriplets] = Triplet(pivot_idx,j,i);
                }
                ntriplets++;
            }
        }

    // Debug printout
    //std::cout << "---> Ref Triplets:" << std::endl;
    //for (unsigned int i = 0; i < ntriplets; i++)
    //    std::cout << "     - triplet: " << triplets[i].idx0 << "-" << triplets[i].idx1 << "-" << triplets[i].idx2 << std::endl;

    // Filter triplets
    for (unsigned int i = 0; i < NTRIPLETS_MAX; i++)
    {
        // idxs
        unsigned int idx0 = triplets[i].idx0;
        unsigned int idx1 = triplets[i].idx1;
        unsigned int idx2 = triplets[i].idx2;

        // Add selections on sum-charge, pt triplet, mass triplet
        masked_triplets[i] = ( std::abs(input[idx0].charge()+input[idx1].charge()+input[idx2].charge()) != 1 ||
                              (input[idx0].hwPt < 15) || (input[idx1].hwPt < 4) || (input[idx2].hwPt < 3)
                            );
    }

    // Debug printout
    //std::cout << "     Ref Cleaned Triplets:" << std::endl;
    //for (unsigned int i = 0; i < NTRIPLETS_MAX; i++)
    //    if (!masked_triplets[i])
    //        std::cout << "     - triplet: " << triplets[i].idx0 << "-" << triplets[i].idx1 << "-" << triplets[i].idx2 << std::endl;
}
