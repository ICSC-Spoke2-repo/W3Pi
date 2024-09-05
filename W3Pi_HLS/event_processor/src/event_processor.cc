#include "event_processor.h"
#ifndef __SYNTHESIS__
#include <cstdio>
#endif

template<int N>
Puppi::pt_t SumReduce(const Puppi::pt_t in[N]) {
       return SumReduce<N/2>(in) + SumReduce<N-N/2>(&in[N/2]);
}

template<>
Puppi::pt_t SumReduce<1>(const Puppi::pt_t in[1]) {
    return in[0];
}

Puppi::pt_t SumReduceAll(const Puppi::pt_t in[NPUPPI_MAX]) {
    #pragma HLS inline off
    #pragma HLS pipeline ii=1
    return SumReduce<NPUPPI_MAX>(in);
}

ap_int<Puppi::eta_t::width+1> deltaEta(Puppi::eta_t eta1, Puppi::eta_t eta2) {
    #pragma HLS latency min=1
    #pragma HLS inline off
    return eta1 - eta2;
}

inline dr2_t deltaR2(const Puppi & p1, const Puppi & p2) {
    // Compute dPhi with protections against values above pi
    auto dphi = p1.hwPhi - p2.hwPhi;
    if (dphi > Puppi::INT_PI)
        dphi -= Puppi::INT_2PI;
    else if (dphi < -Puppi::INT_PI)
        dphi += Puppi::INT_2PI;

    // Compute delta Eta
    auto deta = deltaEta(p1.hwEta, p2.hwEta);

    // Return dR2
    return dphi*dphi + deta*deta;
}

bool BestSeedIdx2(Puppi a, bool a_masked, Puppi b, bool b_masked) {
    bool bestByPt = (a.hwPt >= b.hwPt);
    return (!a_masked && (bestByPt || b_masked)) ? true : false;
}

template<int width>
int BestSeedReduceIdx(const Puppi x[width], const bool masked[width], const int my_indexes[width]) {
    // Tree reduce from https://github.com/definelicht/hlslib/blob/master/include/hlslib/xilinx/TreeReduce.h
    #pragma HLS inline

    // Compute half width of input arrays
    static constexpr int halfWidth = width / 2;
    static constexpr int reducedSize = halfWidth + width % 2;

    // Define reduced size arrays
    Puppi reduced[reducedSize];
    bool maskreduced[reducedSize];
    int indexesreduced[reducedSize];
    #pragma HLS array_partition variable=reduced complete
    #pragma HLS array_partition variable=maskreduced complete
    #pragma HLS array_partition variable=indexesreduced complete

    // Loop to compare elements in pairs
    LOOP_BSRI: for (int i = 0; i < halfWidth; ++i) {
        #pragma HLS unroll
        bool first_best = BestSeedIdx2(x[i*2], masked[i*2], x[i*2+1], masked[i*2+1]);
        reduced[i] = first_best ? x[i*2] : x[i*2+1];
        indexesreduced[i] = first_best ? my_indexes[i*2]: my_indexes[i*2+1];
        maskreduced[i] = (masked[i*2] && masked[i*2+1]);
    }
    // if input particles are odd, the last particle hasn't been checked/compared, do it now
    if(halfWidth != reducedSize){
        reduced[reducedSize - 1] = x[width - 1];
        indexesreduced[reducedSize - 1] = my_indexes[width - 1];
        maskreduced[reducedSize - 1] = masked[width - 1];
    }
    // Recursive call with reduced particles (only the ones that passes the first comparison)
    return BestSeedReduceIdx<reducedSize>(reduced, maskreduced, indexesreduced);
}

template<>
int BestSeedReduceIdx<2>(const Puppi x[2], const bool masked[2], const int my_indexes[2]) {
    #pragma HLS inline
    return BestSeedIdx2(x[0], masked[0], x[1], masked[1]) ? my_indexes[0] : my_indexes[1];
}

int find_pivot_idx(const Puppi in[NPUPPI_MAX], const bool masked[NPUPPI_MAX], const int my_indexes[NPUPPI_MAX]) {
    #pragma HLS inline off
	#pragma HLS pipeline ii=1
    return BestSeedReduceIdx<NPUPPI_MAX>(in, masked, my_indexes);
}

// Apply selections to filter only good seed-candidates:
//  - pdgID = +/- 211 || +/- 11
//  - charge = +/-1 (automatically included in ID check)
//  - pt > 3
//  - -2.4 <= eta <= 2.4
void filter_candidates(const Puppi input[NPUPPI_MAX], bool masked[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=masked complete
    #pragma HLS pipeline II=1

    // Loop on candidates and apply selections
    LOOP_FC: for (unsigned int i = 0; i < NPUPPI_MAX; i++)
    {
        bool badID  = input[i].hwID <= 1 || input[i].hwID >= 6;
        bool badPt  = input[i].hwPt <= 3;
        bool badEta = input[i].hwEta < -Puppi::ETA_CUT || input[i].hwEta > Puppi::ETA_CUT;
        masked[i] = (badID || badPt || badEta);
        // Debug printout
        //if (i < 5)
        //{
        //    std::cout << " - Candidate: " << i << " - badID: " << badID << " badPt: " << badPt << " badEta: " << badEta << " - masked:" << masked[i] << std::endl;
        //    std::cout << "            pT: " << input[i].hwPt << " eta: " << input[i].hwEta*Puppi::ETAPHI_LSB << " ID: " << input[i].hwID << std::endl;
        //    std::cout << "            eta_raw: " << input[i].hwEta << " ETA_CUT_raw: " << Puppi::ETA_CUT << std::endl;
        //}
    }
    // Debug printout
    //for (unsigned int i = 0; i < 5; i++) std::cout << " - Filtered Candidate: " << i << " - masked ? " << masked[i] << std::endl;
}

// Filter triplets
//  - sum of the charges = +/-1
//  - pT >= 15/4/3 GeV
//  - 50 <= mass_triplet <= 110
void filter_triplets(const Puppi input[NPUPPI_MAX], const Triplet triplets[NTRIPLETS_MAX], bool masked_triplets[NTRIPLETS_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=triplets complete
    #pragma HLS ARRAY_PARTITION variable=masked_triplets complete
    #pragma HLS pipeline II=1

    LOOP_FT: for (unsigned int i = 0; i < NTRIPLETS_MAX; i++)
    {
        // Get indexes
        unsigned int idx0 = triplets[i].idx0;
        unsigned int idx1 = triplets[i].idx1;
        unsigned int idx2 = triplets[i].idx2;

        // Add selections on sum-charge, pt triplet, mass triplet
        masked_triplets[i] = ( std::abs(input[idx0].charge()+input[idx1].charge()+input[idx2].charge()) != 1 ||
                              (input[idx0].hwPt < 15) || (input[idx1].hwPt < 4) || (input[idx2].hwPt < 3)
                              // FIXME: add mass selection here
                            );
    }
}


// For each filtered candidate compute iso_sum (sum of pts)
Puppi::pt_t get_iso(const Puppi input[NPUPPI_MAX], const bool masked[NPUPPI_MAX], const dr2_t dr2_max, const dr2_t dr2_veto, const Puppi seed)
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=masked complete
    //#pragma HLS pipeline II=9

    // Debug printout
    //std::cout << "  - Inside pT: " << seed.hwPt << " eta: " << seed.hwEta*Puppi::ETAPHI_LSB << " ID: " << seed.hwID << std::endl;

    // Define list of pts to sum (particle inside isolation cone)
    Puppi::pt_t tosum[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=tosum complete

    // Loop on all particles to compute iso_sum
    LOOP_GI: for (unsigned int i = 0; i < NPUPPI_MAX; ++i) {
        #pragma HLS UNROLL

        // Get dR2
        dr2_t dr2 = deltaR2(seed, input[i]);

        // Check if paricle is inside isolation cone
        bool inside = (dr2 < dr2_max);

        // If inside and not in veto cone, get pt for iso computation
        tosum[i] = inside && (dr2 > dr2_veto) ? input[i].hwPt : Puppi::pt_t(0);

        // Debug printout
        //if (i < 5) std::cout << "      Part " << i << " dR2: " << dr2 << " inside: " << inside << " veto: " << (dr2 <= dr2_veto) << " tosum: " << tosum[i] << std::endl;
    }

    // Compute final sum to get iso_sum
    Puppi::pt_t iso_sum = SumReduceAll(tosum);
    // Debug printout
    //std::cout << "    iso_sum: " << iso_sum << std::endl;

    return iso_sum;
}

// Compute isolation for all filtered candidates candidates
void compute_isolation(const Puppi input[NPUPPI_MAX], const bool masked[NPUPPI_MAX], Puppi::pt_t output_absiso[NPUPPI_MAX], dr2_t dr2_max, dr2_t dr2_veto)
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=masked complete
    #pragma HLS ARRAY_PARTITION variable=output_absiso complete
    //#pragma HLS pipeline II=9

    // Compute isolation for all (filtered) candidates
    LOOP_CI: for (unsigned int j = 0; j < NPUPPI_MAX; ++j)
        output_absiso[j] = masked[j] ? Puppi::pt_t(0) : get_iso(input, masked, dr2_max, dr2_veto, input[j]);
}

// Top function:
//  - filter candidates
//  - add isolation to filtered candidates
//  - update mask to consider only (iso_sum/pt) <= 0.6
//  - find pivot among them
void event_processor (const Puppi input[NPUPPI_MAX], Puppi & pivot, Triplet triplets[NTRIPLETS_MAX], bool masked_triplets[NTRIPLETS_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=triplets complete
    #pragma HLS ARRAY_PARTITION variable=masked_triplets complete
    //#pragma HLS pipeline II=9

    // Define masked lists to filter candidates
    bool masked[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=masked complete

    // Filter candidates
    filter_candidates(input, masked);

    // Define isolation array
    Puppi::pt_t output_absiso[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=output_absiso complete

    // Clear isolation array
    LOOP_EP1: for (unsigned int j = 0; j < NPUPPI_MAX; ++j)
    {
        #pragma HLS UNROLL
        output_absiso[j] = 0;
    }

    // Define min/max isolation cones
    const dr2_t dr2_max = drToHwDr2(0.4), dr2_veto = drToHwDr2(0.1);

    // Compute abs isolation for filter candidates
    compute_isolation(input, masked, output_absiso, dr2_max, dr2_veto);

    // Update mask to consider only (iso_sum/pt) <= 0.6
    LOOP_EP2: for (unsigned int i = 0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        masked[i] = masked[i] ? masked[i] : (output_absiso[i]/input[i].hwPt) > 0.6;
        //masked[i] = masked[i] || (output_absiso[i]/input[i].hwPt > 0.6); // Possible alternative, but I see no gain in performance from the report
    }

    // Debug printout
    //for (unsigned int i = 0; i < 45; i++)
    //    if (!masked[i])
    //        std::cout << " -> Seed " << i << " pT: " << input[i].hwPt << " eta: " << input[i].hwEta*Puppi::ETAPHI_LSB << " pdgID: " << input[i].hwID
    //                << " charge: " << input[i].charge() << " IsoSum: " << output_absiso[i] << " Masked: " << masked[i] << std::endl;

    // Indexes array
    int my_indexes[NPUPPI_MAX];
    //#pragma HLS ARRAY_PARTITION variable=my_indexes complete
    LOOP_EP3: for (unsigned int i=0; i < NPUPPI_MAX; i++)
        my_indexes[i] = i;

    // Find pivot (charged filtered candidate with highest pt)
    int pivot_idx = find_pivot_idx(input, masked, my_indexes);
    pivot = input[pivot_idx];

    // Debug printout
    //std::cout << "---> Pivot     idx: " << pivot_idx << std::endl;
    //std::cout << "     Pivot     pT : " << pivot.hwPt << " eta: " << pivot.hwEta*Puppi::ETAPHI_LSB << " pdgID: " << pivot.hwID << std::endl;

    // Build all triplets (pT ordered) starting from pivot
    int ntriplets = 0;
    LOOP_EP4: for (unsigned int i = 0; i < NPUPPI_MAX-1; i++)
    {
        LOOP_EP5: for (unsigned int j = i+1; j < NPUPPI_MAX; j++)
        {
            if (ntriplets == NTRIPLETS_MAX)
                break;

            if (i == pivot_idx || masked[i] || j == pivot_idx || masked[j])
                continue;

            triplets[ntriplets] = (input[i].hwPt >= input[j].hwPt) ? Triplet(pivot_idx,i,j) : Triplet(pivot_idx,j,i);
            ntriplets++;
        }
    }

    // Debug printout
    //std::cout << "---> My Triplets:" << std::endl;
    //for (unsigned int i = 0; i < ntriplets; i++)
    //    std::cout << "     - triplet: " << triplets[i].idx0 << "-" << triplets[i].idx1 << "-" << triplets[i].idx2 << std::endl;

    // Filter triplets
    filter_triplets(input, triplets, masked_triplets);

    // Debug printout
    //std::cout << "     My Cleaned Triplets:" << std::endl;
    //for (unsigned int i = 0; i < NTRIPLETS_MAX; i++)
    //    if (!masked_triplets[i])
    //        std::cout << "     - triplet: " << triplets[i].idx0 << "-" << triplets[i].idx1 << "-" << triplets[i].idx2 << std::endl;

}
