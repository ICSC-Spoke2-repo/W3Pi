#include "event_preparator.h"
#ifndef __SYNTHESIS__
#include <cstdio>
#endif

Puppi BestSeed(Puppi a, bool a_masked, Puppi b, bool b_masked) {
    bool bestByPt = (a.hwPt >= b.hwPt);
    return (!a_masked && (bestByPt || b_masked)) ? a : b;
    // FIXME: missing a check if both a and b are masked?
}

template<int width>
Puppi BestSeedReduce(const Puppi x[width], const bool masked[width]) {
    // Tree reduce from https://github.com/definelicht/hlslib/blob/master/include/hlslib/xilinx/TreeReduce.h
    #pragma HLS inline

    // Split input arrays in 2
    static constexpr int halfWidth = width / 2;  // FIXME: how can we get rid of this division?
    static constexpr int reducedSize = halfWidth + width % 2;

    // Define reduced size arrays
    Puppi reduced[reducedSize];
    bool maskreduced[reducedSize];
    #pragma HLS array_partition variable=reduced complete
    #pragma HLS array_partition variable=maskreduced complete

    // Loop to compare elements in pairs
    for(int i = 0; i < halfWidth; ++i) {
        #pragma HLS unroll
        // Compare elements i and i+1, (valid for both even and odd cases) and save the best (high pt unmasked)
        reduced[i] = BestSeed(x[i*2], masked[i*2], x[i*2+1], masked[i*2+1]);
        maskreduced[i] = (masked[i*2] && masked[i*2+1]);
    }
    // if input particles are odd, the last particle hasn't been checked/compared, do it now
    if(halfWidth != reducedSize){
        reduced[reducedSize - 1] = x[width - 1];
        maskreduced[reducedSize - 1] = masked[width - 1];
    }
    // Recursive call with reduced particles (only the ones that passes the first comparison)
    return BestSeedReduce<reducedSize>(reduced, maskreduced);
}

template<>
Puppi BestSeedReduce<2>(const Puppi x[2], const bool masked[2]) {
    #pragma HLS inline
    return BestSeed(x[0], masked[0], x[1], masked[1]);
}

template<int N>
Puppi::pt_t SumReduce(const Puppi::pt_t in[N]) {
        // Recursive calls on first half and second half of the events
        // FIXME: I don't understand how this splitting is implemented
       return SumReduce<N/2>(in) + SumReduce<N-N/2>(&in[N/2]);
}

template<>
Puppi::pt_t SumReduce<1>(const Puppi::pt_t in[1]) {
    return in[0];
}

Puppi find_seed(const Puppi in[NPUPPI_MAX], const bool masked[NPUPPI_MAX]) {
    #pragma HLS inline off
	#pragma HLS pipeline ii=1
    return BestSeedReduce<NPUPPI_MAX>(in, masked);
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

void one_iteration(const Puppi in[NPUPPI_MAX], const bool masked[NPUPPI_MAX], bool masked_out[NPUPPI_MAX], Puppi & seed, Puppi::pt_t & iso_sum, dr2_t dr2_max, dr2_t dr2_veto) {
    #pragma HLS ARRAY_PARTITION variable=in complete
    #pragma HLS ARRAY_PARTITION variable=masked complete
    #pragma HLS ARRAY_PARTITION variable=masked_out complete
    #pragma HLS pipeline II=9

    #pragma HLS inline off

    // Define list of pts to sum (particle inside isolation cone)
    Puppi::pt_t tosum[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=tosum complete

    // Find seed (charged part with highest pt)
    seed = find_seed(in, masked);

    // Loop on all particles to compute iso of seed
    // FIXME: avoid the comparison with the seed itself? (maybe using seed index...)
    for (unsigned int i = 0; i < NPUPPI_MAX; ++i) {
        // Get dR2
        dr2_t dr2 = deltaR2(seed, in[i]);
        // Check if paricle is inside isolation cone
        bool inside = (dr2 < dr2_max);
        // if inside (used for this seed) then mask it for next iterations
        masked_out[i] = masked[i] || inside;
        // if inside and not in veto cone, get pt for iso computation
        tosum[i] =  inside && (dr2 > dr2_veto) ? in[i].hwPt : Puppi::pt_t(0);
    }
    iso_sum = SumReduceAll(tosum);
}

void compute_isolated_w3p(const Puppi in[NPUPPI_MAX], Puppi out[NISO_MAX], Puppi::pt_t out_absiso[NISO_MAX])  {
    #pragma HLS ARRAY_PARTITION variable=in complete
    #pragma HLS ARRAY_PARTITION variable=out complete
    #pragma HLS ARRAY_PARTITION variable=out_absiso complete
    #pragma HLS pipeline II=9

    // Define min/max isolation cones
    const dr2_t dr2_max = drToHwDr2(0.4), dr2_veto = drToHwDr2(0.1);

    // Define masked lists
    bool masked[NPUPPI_MAX], masked_out[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=masked complete
    #pragma HLS ARRAY_PARTITION variable=masked_out complete

    // Define
    Puppi::pt_t tosum[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=tosum complete

    // Mask neutral particles
    for (unsigned int i = 0; i < NPUPPI_MAX; ++i) {
        masked[i] = in[i].hwID <= 1;
    }

    // Clear output arrays
    for (unsigned int j = 0; j < NISO_MAX; ++j) {
        out[j].clear();
        out_absiso[j] = 0;
    }

    int niso = 0;
    for (unsigned int j = 0; j < NISO_MAX; ++j) {
        // Define seed (candidate output isolated particle) and its iso
        Puppi seed;
        Puppi::pt_t iso_sum;

        // Do actual processing here for this seed
        one_iteration(in, masked, masked_out, seed, iso_sum, dr2_max, dr2_veto);

        // Mask also vetoed particles
        for (unsigned int i = 0; i < NPUPPI_MAX; ++i) {
            masked[i] = masked_out[i];
        }
#ifndef __SYNTHESIS__
        // Debug printout
        //printf("HW Seed pT %8.3f eta %+6.3f phi %+6.3f pid %1u: abs iso %8.3f\n",
        //    seed.floatPt(), seed.floatEta(), seed.floatPhi(), seed.hwID.to_uint(), Puppi::floatPt(iso_sum));
#endif
        // If seed is isolated, store it in the output
        if (iso_sum < seed.hwPt) {
            out[niso] = seed;
            out_absiso[niso] = iso_sum;
            niso++;
        }
    }

}

// ------------------------------------------------------------------------------------------------------------------------------
//      ***  NEW SECTION  ***
// ------------------------------------------------------------------------------------------------------------------------------

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
    for (unsigned int i = 0; i < NPUPPI_MAX; i++)
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
    for (unsigned int i = 0; i < NPUPPI_MAX; ++i) {
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
    for (unsigned int j = 0; j < NPUPPI_MAX; ++j)
        output_absiso[j] = masked[j] ? Puppi::pt_t(0) : get_iso(input, masked, dr2_max, dr2_veto, input[j]);
}

// Top function:
//  - filter candidates
//  - add isolation to filtered candidates
//  - update mask to consider only (iso_sum/pt) <= 0.6
//  - find pivot among them
void event_preparator (const Puppi input[NPUPPI_MAX], Puppi & pivot)
{
    #pragma HLS ARRAY_PARTITION variable=input complete

    // Define masked lists to filter candidates
    bool masked[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=masked complete

    // Filter candidates
    filter_candidates(input, masked);

    // Define isolation array
    Puppi::pt_t output_absiso[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=output_absiso complete

    // Clear isolation array
    for (unsigned int j = 0; j < NPUPPI_MAX; ++j)
        output_absiso[j] = 0;

    // Define min/max isolation cones
    const dr2_t dr2_max = drToHwDr2(0.4), dr2_veto = drToHwDr2(0.1);

    // Compute abs isolation for filter candidates
    compute_isolation(input, masked, output_absiso, dr2_max, dr2_veto);

    // Update mask to consider only (iso_sum/pt) <= 0.6
    for (unsigned int i = 0; i < NPUPPI_MAX; i++)
    {
        masked[i] = masked[i] ? masked[i] : (output_absiso[i]/input[i].hwPt) > 0.6;
    }

    // Debug printout
    for (unsigned int i = 0; i < 45; i++)
        if (!masked[i])
            std::cout << " -> Seed " << i << " pT: " << input[i].hwPt << " eta: " << input[i].hwEta*Puppi::ETAPHI_LSB << " ID: " << input[i].hwID
                    << " charge: " << input[i].charge() << " IsoSum: " << output_absiso[i] << " Masked: " << masked[i] << std::endl;

    // Find pivot (charged filtered candidate with highest pt)
    //Puppi pivot = find_seed(input, masked);
    pivot = find_seed(input, masked); // FIXME: use line above when implementing next part (remove pivot from arguments)

    // Debug printout
    std::cout << "---> Pivot    pT: " << pivot.hwPt << " eta: " << pivot.hwEta*Puppi::ETAPHI_LSB << " ID: " << pivot.hwID << std::endl;
}
