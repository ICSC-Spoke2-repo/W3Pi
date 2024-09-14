#include "src/event_processor.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <bitset>

#define DEBUG 1
#define DEEP_DEBUG 0
#define OUTPUT_DEBUG 1
#define NTEST 2

// DUTs:
//  1  : Masker
//  2  : Slimmer
//  3  : Orderer
//  31 : Orderer3
//  32 : Orderer4
//  33 : Orderer5
//  35 : Orderer7
//  36 : Orderer8
//  37 : Orderer7bis
//  4  : Merger
//  41 : Merger7bis
//  5  : get_triplet_inputs
//  51 : get_event_inputs
//  52 : get_cos_phi / get_cosh_eta
//  53 : get_pair_mass
//  6  : get_event_scores
//  7  : get_highest_score
//  10 : EventProcessor
//  11 : EventProcessor7bis
//  12 : EventProcessor7f
#define DUT 53

// Pretty print of array
template<typename T>
void printArray(T A[], int size)
{
    std::cout << " ";
    for (unsigned int i = 0; i < size; i++)
        std::cout << A[i] << " ";
    std::cout << std::endl;
}

// Main testbench function
int main(int argc, char **argv) {

    // Variables to read data
    uint64_t header, data[NPUPPI_MAX];

    // Read input stream
    //std::fstream in("Puppi_SingleNu.dump", std::ios::in | std::ios::binary);
    //std::fstream in("Puppi_w3p_PU0.dump", std::ios::in | std::ios::binary);
    std::fstream in("Puppi_w3p_PU200.dump", std::ios::in | std::ios::binary);

    // Loop on input data in chunks
    for (int itest = 0, ntest = NTEST; itest < ntest && in.good(); ++itest) {

        std::cout << "--------------------" << std::endl;

        // Read header
        in.read(reinterpret_cast<char *>(&header), sizeof(uint64_t));

        // Read header quantities
        // bits 	size 	meaning
        // 63-62 	2 	    10 = valid event header
        // 61 	    1 	    error bit
        // 60-56 	6 	    (local) run number
        // 55-24 	32 	    orbit number
        // 23-12 	12 	    bunch crossing number (0-3563)
        // 11-07 	4 	    must be set to 0
        // 07-00 	8 	    number of Puppi candidates
        unsigned int npuppi =  header        & 0xFF      ; // 8 bits
        unsigned int beZero = (header >> 8)  & 0xF       ; // 4 bits
        unsigned int bxNum  = (header >> 12) & 0xFFF     ; // 12 bits
        unsigned int orbitN = (header >> 24) & 0xFFFFFFFF; // 32 bits
        unsigned int runN   = (header >> 56) & 0x3F      ; // 6 bits
        unsigned int error  = (header >> 62) & 0x1       ; // 1
        unsigned int validH = (header >> 63) & 0x3       ; // 2 bits

        // Print header quantities
        std::cout << "*** itest " << itest << " / ntest " << ntest << " (npuppi = " << npuppi << ")" << std::endl;
        if (DEEP_DEBUG)
        {
            std::cout << " - header: " << std::bitset<64>(header) << " : " << header << std::endl;
            std::cout << " - npuppi: " << std::bitset<64>(npuppi) << " : " << npuppi << std::endl;
            std::cout << " - beZero: " << std::bitset<64>(beZero) << " : " << beZero << std::endl;
            std::cout << " - bxNum : " << std::bitset<64>(bxNum)  << " : " << bxNum  << std::endl;
            std::cout << " - orbitN: " << std::bitset<64>(orbitN) << " : " << orbitN << std::endl;
            std::cout << " - runN  : " << std::bitset<64>(runN  ) << " : " << runN   << std::endl;
            std::cout << " - error : " << std::bitset<64>(error ) << " : " << error  << std::endl;
            std::cout << " - validH: " << std::bitset<64>(validH) << " : " << validH << std::endl;
        }

        // Minimal assert on npuppi to guarantee correct reading of fstream
        assert(npuppi <= NPUPPI_MAX);
        if (npuppi == 0) continue;

        // Read actual data and store it in puppi array
        in.read(reinterpret_cast<char *>(data), npuppi*sizeof(uint64_t));

        // Use only events with at least 3 puppi candidates
        // FIXME this should eventually be moved to the firmware, see:
        //       https://github.com/gpetruc/GlobalCorrelator_HLS/tree/tutorial-2023/4.stateful
        if (npuppi < 3) continue;
        //if (itest != 4) continue;

        // Declare data variables
        Puppi inputs[NPUPPI_MAX];

        // Loop on puppi candidates
        if (DEBUG) std::cout << " - Particles:" << std::endl;
        for (unsigned int i = 0; i < npuppi; ++i)
        {
            // Unpack data into puppi candidates
            inputs[i].unpack(data[i]);

            // Debug printouts of puppi candidates
            //if (itest == 0 && DEBUG)
            if (DEBUG)
            {
                printf("  %3u/%3u : pT %6.2f  eta %+6.3f  phi %+6.3f  pid %1u  Z0 %+6.3f\n",
                        i, npuppi, inputs[i].floatPt(), inputs[i].floatEta(),
                        inputs[i].floatPhi(), inputs[i].hwID.to_uint(), inputs[i].floatZ0());
                if (DEEP_DEBUG)
                {
                    ap_uint<64> casted = ap_uint<64>(data[i]);
                    std::cout << "    |_ casted: " << std::bitset<64>(casted) << std::endl;
                    std::cout << "    |_ pT    : " << std::bitset<14>(casted(13,0))  << std::endl;
                    std::cout << "    |_ eta   : " << std::bitset<12>(casted(25,14)) << std::endl;
                    std::cout << "    |_ phi   : " << std::bitset<11>(casted(36,26)) << std::endl;
                    std::cout << "    |_ PID   : " << std::bitset<3> (casted(39,37)) << std::endl;
                    std::cout << "    |_ Z0    : " << std::bitset<10>(casted(49,40)) << std::endl;
                    std::cout << "    |_ rest  : " << std::bitset<14>(casted(63,50)) << std::endl;
                }
            }
        }
        // Clear remaining particles in array
        for (unsigned int i = npuppi; i < NPUPPI_MAX; ++i)
        {
            inputs[i].clear();
        }

        // Do the processing

        // Outputs
        ap_uint<NPUPPI_MAX> masked_fw;
        ap_uint<NPUPPI_MAX> masked_ref;
        //idx_t slimmed_fw[NPUPPI_SEL];
        //idx_t slimmed_ref[NPUPPI_SEL];
        Puppi slimmed_fw[NPUPPI_MAX];
        Puppi slimmed_ref[NPUPPI_MAX];
        //idx_t ordered_idxs_fw[NPUPPI_SEL];
        //idx_t ordered_idxs_ref[NPUPPI_SEL];
        idx_t ordered_idxs_fw[NPUPPI_MAX];
        idx_t ordered_idxs_ref[NPUPPI_MAX];
        //idx_t ordered_idxs_fw1[NPUPPI_MAX/4], ordered_idxs_fw2[NPUPPI_MAX/4], ordered_idxs_fw3[NPUPPI_MAX/4], ordered_idxs_fw4[NPUPPI_MAX/4];
        //idx_t ordered_idxs_ref1[NPUPPI_MAX/4], ordered_idxs_ref2[NPUPPI_MAX/4], ordered_idxs_ref3[NPUPPI_MAX/4], ordered_idxs_ref4[NPUPPI_MAX/4];
        idx_t ordered_idxs_fw1[NPUPPI_MAX/8], ordered_idxs_fw2[NPUPPI_MAX/8], ordered_idxs_fw3[NPUPPI_MAX/8], ordered_idxs_fw4[NPUPPI_MAX/8],
              ordered_idxs_fw5[NPUPPI_MAX/8], ordered_idxs_fw6[NPUPPI_MAX/8], ordered_idxs_fw7[NPUPPI_MAX/8], ordered_idxs_fw8[NPUPPI_MAX/8];
        idx_t ordered_idxs_ref1[NPUPPI_MAX/8], ordered_idxs_ref2[NPUPPI_MAX/8], ordered_idxs_ref3[NPUPPI_MAX/8], ordered_idxs_ref4[NPUPPI_MAX/8],
              ordered_idxs_ref5[NPUPPI_MAX/8], ordered_idxs_ref6[NPUPPI_MAX/8], ordered_idxs_ref7[NPUPPI_MAX/8], ordered_idxs_ref8[NPUPPI_MAX/8];
        Puppi ordered_fw1[NSPLITS] , ordered_fw2[NSPLITS] , ordered_fw3[NSPLITS] , ordered_fw4[NSPLITS],
              ordered_fw5[NSPLITS] , ordered_fw6[NSPLITS] , ordered_fw7[NSPLITS] , ordered_fw8[NSPLITS],
              ordered_fw9[NSPLITS] , ordered_fw10[NSPLITS], ordered_fw11[NSPLITS], ordered_fw12[NSPLITS],
              ordered_fw13[NSPLITS], ordered_fw14[NSPLITS], ordered_fw15[NSPLITS], ordered_fw16[NSPLITS];
        Puppi ordered_ref1[NSPLITS] , ordered_ref2[NSPLITS] , ordered_ref3[NSPLITS] , ordered_ref4[NSPLITS],
              ordered_ref5[NSPLITS] , ordered_ref6[NSPLITS] , ordered_ref7[NSPLITS] , ordered_ref8[NSPLITS],
              ordered_ref9[NSPLITS] , ordered_ref10[NSPLITS], ordered_ref11[NSPLITS], ordered_ref12[NSPLITS],
              ordered_ref13[NSPLITS], ordered_ref14[NSPLITS], ordered_ref15[NSPLITS], ordered_ref16[NSPLITS];
        Puppi merged_fw[NPUPPI_MAX];
        Puppi merged_ref[NPUPPI_MAX];
        Puppi ordered_fw[NPUPPI_MAX];
        Puppi ordered_ref[NPUPPI_MAX];
        Puppi ordered2_fw[NSUBARR][NSPLITS];
        idx_t selected_idxs_fw[NPUPPI_SEL];
        idx_t selected_idxs_ref[NPUPPI_SEL];
        Puppi selected_fw[NPUPPI_SEL];
        Puppi selected_ref[NPUPPI_SEL];
        Puppi final_fw[NPUPPI_SEL];
        Puppi final_ref[NPUPPI_SEL];
        cos_t cosphi;
        cosh_t cosheta;
        mass_t mass_fw;
        mass_t mass_ref;
        w3p_bdt::input_t inputs_fw[w3p_bdt::n_features];
        w3p_bdt::input_t inputs_ref[w3p_bdt::n_features];
        w3p_bdt::input_t BDT_inputs_fw[NTRIPLETS][w3p_bdt::n_features];
        w3p_bdt::input_t BDT_inputs_ref[NTRIPLETS][w3p_bdt::n_features];
        w3p_bdt::score_t BDT_scores_fw[NTRIPLETS];
        w3p_bdt::score_t BDT_scores_ref[NTRIPLETS];
        w3p_bdt::score_t max_score_fw;
        w3p_bdt::score_t max_score_ref;

        idx_t all_idxs[NPUPPI_MAX];
        for (unsigned int i=0; i < NPUPPI_MAX; i++)
            all_idxs[i] = i;

        // FIRMWARE & REFERENCE call
        if (DUT == 1)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);
        }
        else if (DUT == 2)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            //slimmer(masked_fw, slimmed_fw);
            //slimmer_ref(masked_ref, slimmed_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);
        }
        else if (DUT == 3)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            //orderer(inputs, slimmed_fw, ordered_idxs_fw);
            //orderer_ref(inputs, slimmed_ref, ordered_idxs_ref);
            orderer2(slimmed_fw, ordered_idxs_fw);
            orderer2_ref(slimmed_ref, ordered_idxs_ref);
        }
        else if (DUT == 31)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer3(slimmed_fw, ordered_idxs_fw1, ordered_idxs_fw2, ordered_idxs_fw3, ordered_idxs_fw4);
            orderer3_ref(slimmed_ref, ordered_idxs_ref1, ordered_idxs_ref2, ordered_idxs_ref3, ordered_idxs_ref4);
        }
        else if (DUT == 32)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer4(slimmed_fw, ordered_idxs_fw1, ordered_idxs_fw2, ordered_idxs_fw3, ordered_idxs_fw4,
                                 ordered_idxs_fw5, ordered_idxs_fw6, ordered_idxs_fw7, ordered_idxs_fw8);
            orderer4_ref(slimmed_ref, ordered_idxs_ref1, ordered_idxs_ref2, ordered_idxs_ref3, ordered_idxs_ref4,
                                      ordered_idxs_ref5, ordered_idxs_ref6, ordered_idxs_ref7, ordered_idxs_ref8);
        }
        else if (DUT == 33)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer5(slimmed_fw, ordered_fw);
            //orderer5_ref(slimmed_ref, ordered_ref);
        }
        else if (DUT == 35)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                 ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                                 ordered_fw9, ordered_fw10, ordered_fw11, ordered_fw12,
                                 ordered_fw13, ordered_fw14, ordered_fw15, ordered_fw16);
            orderer7_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                      ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8,
                                      ordered_ref9, ordered_ref10, ordered_ref11, ordered_ref12,
                                      ordered_ref13, ordered_ref14, ordered_ref15, ordered_ref16);
        }
        else if (DUT == 36)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer8(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                 ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                                 ordered_fw9, ordered_fw10, ordered_fw11, ordered_fw12,
                                 ordered_fw13, ordered_fw14, ordered_fw15, ordered_fw16);
            orderer7_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                      ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8,
                                      ordered_ref9, ordered_ref10, ordered_ref11, ordered_ref12,
                                      ordered_ref13, ordered_ref14, ordered_ref15, ordered_ref16);
        }
        else if (DUT == 37)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            //orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
            //                        ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            //orderer7c(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
            //                        ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            //orderer7d(slimmed_fw, ordered_fw);
            //orderer7e(slimmed_fw, ordered2_fw);
            //orderer7f(slimmed_fw, ordered2_fw);
            orderer7g(slimmed_fw, ordered2_fw);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);
        }
        else if (DUT == 4)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                 ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                                 ordered_fw9, ordered_fw10, ordered_fw11, ordered_fw12,
                                 ordered_fw13, ordered_fw14, ordered_fw15, ordered_fw16);
            orderer7_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                      ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8,
                                      ordered_ref9, ordered_ref10, ordered_ref11, ordered_ref12,
                                      ordered_ref13, ordered_ref14, ordered_ref15, ordered_ref16);

            merger(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                   ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                   ordered_fw9, ordered_fw10, ordered_fw11, ordered_fw12,
                   ordered_fw13, ordered_fw14, ordered_fw15, ordered_fw16,
                   merged_fw);
            merger_ref(slimmed_ref, merged_ref);
        }
        else if (DUT == 41)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            //orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
            //                        ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7f(slimmed_fw, ordered2_fw);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            //merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
            //           ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
            //           merged_fw);
            merger7f(ordered2_fw, merged_fw);
            merger_ref(slimmed_ref, merged_ref);
        }
        else if (DUT == 5)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                    ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                       ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                       merged_fw);
            merger_ref(slimmed_ref, merged_ref);

            // Get first 10 candidates
            for (unsigned int i = 0; i < NPUPPI_SEL; i++)
            {
                selected_fw[i]  = merged_fw[i];
                selected_ref[i] = merged_ref[i];
            }

            get_triplet_inputs(selected_fw, 0, 1, 2, inputs_fw);
            get_triplet_inputs_ref(selected_ref, 0, 1, 2, inputs_ref);
        }
        else if (DUT == 51)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                    ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                       ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                       merged_fw);
            merger_ref(slimmed_ref, merged_ref);

            // Get first 10 candidates
            for (unsigned int i = 0; i < NPUPPI_SEL; i++)
            {
                selected_fw[i]  = merged_fw[i];
                selected_ref[i] = merged_ref[i];
            }
            
            // Get inputs for each triplet
            get_event_inputs(selected_fw, BDT_inputs_fw);
            get_event_inputs_ref(selected_ref, BDT_inputs_ref);
        }
        else if (DUT == 52)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                    ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                       ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                       merged_fw);
            merger_ref(slimmed_ref, merged_ref);

            // Get first 10 candidates
            for (unsigned int i = 0; i < NPUPPI_SEL; i++)
            {
                selected_fw[i]  = merged_fw[i];
                selected_ref[i] = merged_ref[i];
            }

            cosphi = get_cos_phi(selected_fw[0].hwPhi);
            cosheta = get_cosh_eta(selected_fw[0].hwEta);
        }
        else if (DUT == 53)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                    ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                       ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                       merged_fw);
            merger_ref(slimmed_ref, merged_ref);

            // Get first 10 candidates
            for (unsigned int i = 0; i < NPUPPI_SEL; i++)
            {
                selected_fw[i]  = merged_fw[i];
                selected_ref[i] = merged_ref[i];
            }

            mass_fw  = get_pair_mass(selected_fw[0], selected_fw[1]);
            mass_ref = get_pair_mass_ref(selected_ref[0], selected_ref[1]);
        }
        else if (DUT == 6)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                    ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                       ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                       merged_fw);
            merger_ref(slimmed_ref, merged_ref);

            // Get first 10 candidates
            for (unsigned int i = 0; i < NPUPPI_SEL; i++)
            {
                selected_fw[i]  = merged_fw[i];
                selected_ref[i] = merged_ref[i];
            }

            // Get inputs for each triplet
            get_event_inputs(selected_fw, BDT_inputs_fw);
            get_event_inputs_ref(selected_ref, BDT_inputs_ref);

            // Get BDT score for each triplet
            get_event_scores(BDT_inputs_fw, BDT_scores_fw);
            get_event_scores_ref(BDT_inputs_ref, BDT_scores_ref);
        }
        else if (DUT == 7)
        {
            masker(inputs, masked_fw);
            masker_ref(inputs, masked_ref);

            slimmer2(inputs, masked_fw, slimmed_fw);
            slimmer2_ref(inputs, masked_ref, slimmed_ref);

            orderer7bis(slimmed_fw, ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                                    ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8);
            orderer7bis_ref(slimmed_ref, ordered_ref1, ordered_ref2, ordered_ref3, ordered_ref4,
                                         ordered_ref5, ordered_ref6, ordered_ref7, ordered_ref8);

            merger7bis(ordered_fw1, ordered_fw2, ordered_fw3, ordered_fw4,
                       ordered_fw5, ordered_fw6, ordered_fw7, ordered_fw8,
                       merged_fw);
            merger_ref(slimmed_ref, merged_ref);

            // Get first 10 candidates
            for (unsigned int i = 0; i < NPUPPI_SEL; i++)
            {
                selected_fw[i]  = merged_fw[i];
                selected_ref[i] = merged_ref[i];
            }

            // Get inputs for each triplet
            get_event_inputs(selected_fw, BDT_inputs_fw);
            get_event_inputs_ref(selected_ref, BDT_inputs_ref);

            // Get BDT score for each triplet
            get_event_scores(BDT_inputs_fw, BDT_scores_fw);
            get_event_scores_ref(BDT_inputs_ref, BDT_scores_ref);

            // Get highest BDT score among triplets
            get_highest_score(BDT_scores_fw, max_score_fw);
            get_highest_score_ref(BDT_scores_ref, max_score_ref);
        }
        else if (DUT == 10)
        {
            EventProcessor(inputs, final_fw);
            EventProcessor_ref(inputs, max_score_ref);
        }
        else if (DUT == 11)
        {
            EventProcessor7bis(inputs, max_score_fw);
            EventProcessor_ref(inputs, max_score_ref);
        }
        else if (DUT == 12)
        {
            EventProcessor7f(inputs, max_score_fw);
            EventProcessor_ref(inputs, max_score_ref);
        }

        // Post calls printout
        if (OUTPUT_DEBUG)
        {
            if (DUT == 1)
            {
                std::cout << "- Masked:" << std::endl;
                std::cout << " FW :" << std::bitset<NPUPPI_MAX>(masked_fw) << std::endl;
                std::cout << " REF:" << std::bitset<NPUPPI_MAX>(masked_ref) << std::endl;
            }
            else if (DUT == 2)
            {
                std::cout << "- Slimmed:" << std::endl;
                //std::cout << " FW :"; printArray<idx_t>(slimmed_fw, NPUPPI_SEL);
                //std::cout << " REF:"; printArray<idx_t>(slimmed_ref, NPUPPI_SEL);
                std::cout << " FW :"; printArray<Puppi>(slimmed_fw, NPUPPI_MAX);
                std::cout << " REF:"; printArray<Puppi>(slimmed_ref, NPUPPI_MAX);
            }
            else if (DUT == 3)
            {
                std::cout << "- Ordered indexes" << std::endl;
                std::cout << " FW :"; printArray<idx_t>(ordered_idxs_fw, NPUPPI_MAX);
                std::cout << " REF:"; printArray<idx_t>(ordered_idxs_ref, NPUPPI_MAX);
            }
            else if (DUT == 31)
            {
                std::cout << "- Ordered3 indexes" << std::endl;
                std::cout << " 1 FW :"; printArray<idx_t>(ordered_idxs_fw1 , NPUPPI_MAX/4);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref1, NPUPPI_MAX/4);
                std::cout << " 2 FW :"; printArray<idx_t>(ordered_idxs_fw2 , NPUPPI_MAX/4);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref2, NPUPPI_MAX/4);
                std::cout << " 3 FW :"; printArray<idx_t>(ordered_idxs_fw3 , NPUPPI_MAX/4);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref3, NPUPPI_MAX/4);
                std::cout << " 4 FW :"; printArray<idx_t>(ordered_idxs_fw4 , NPUPPI_MAX/4);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref4, NPUPPI_MAX/4);
            }
            else if (DUT == 32)
            {
                std::cout << "- Ordered4 indexes" << std::endl;
                std::cout << " 1 FW :"; printArray<idx_t>(ordered_idxs_fw1 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref1, NPUPPI_MAX/8);
                std::cout << " 2 FW :"; printArray<idx_t>(ordered_idxs_fw2 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref2, NPUPPI_MAX/8);
                std::cout << " 3 FW :"; printArray<idx_t>(ordered_idxs_fw3 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref3, NPUPPI_MAX/8);
                std::cout << " 4 FW :"; printArray<idx_t>(ordered_idxs_fw4 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref4, NPUPPI_MAX/8);
                std::cout << " 5 FW :"; printArray<idx_t>(ordered_idxs_fw5 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref5, NPUPPI_MAX/8);
                std::cout << " 6 FW :"; printArray<idx_t>(ordered_idxs_fw6 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref6, NPUPPI_MAX/8);
                std::cout << " 7 FW :"; printArray<idx_t>(ordered_idxs_fw7 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref7, NPUPPI_MAX/8);
                std::cout << " 8 FW :"; printArray<idx_t>(ordered_idxs_fw8 , NPUPPI_MAX/8);
                std::cout << "   REF:"; printArray<idx_t>(ordered_idxs_ref8, NPUPPI_MAX/8);

                //// Print pTs for debug purposes
                //Puppi::pt_t pts1[NPUPPI_MAX/8], pts2[NPUPPI_MAX/8], pts3[NPUPPI_MAX/8], pts4[NPUPPI_MAX/8];
                //Puppi::pt_t pts5[NPUPPI_MAX/8], pts6[NPUPPI_MAX/8], pts7[NPUPPI_MAX/8], pts8[NPUPPI_MAX/8];
                //for (unsigned int i=0; i<NPUPPI_MAX/8; i++)
                //{
                //    pts1[i] = slimmed_fw[ordered_idxs_fw1[i]].hwPt;
                //    pts2[i] = slimmed_fw[ordered_idxs_fw2[i]].hwPt;
                //    pts3[i] = slimmed_fw[ordered_idxs_fw3[i]].hwPt;
                //    pts4[i] = slimmed_fw[ordered_idxs_fw4[i]].hwPt;
                //    pts5[i] = slimmed_fw[ordered_idxs_fw5[i]].hwPt;
                //    pts6[i] = slimmed_fw[ordered_idxs_fw6[i]].hwPt;
                //    pts7[i] = slimmed_fw[ordered_idxs_fw7[i]].hwPt;
                //    pts8[i] = slimmed_fw[ordered_idxs_fw8[i]].hwPt;
                //}
                //std::cout << "- Ordered4 pTs" << std::endl;
                //std::cout << " 1 FW :"; printArray<Puppi::pt_t>(pts1, NPUPPI_MAX/8);
                //std::cout << " 2 FW :"; printArray<Puppi::pt_t>(pts2, NPUPPI_MAX/8);
                //std::cout << " 3 FW :"; printArray<Puppi::pt_t>(pts3, NPUPPI_MAX/8);
                //std::cout << " 4 FW :"; printArray<Puppi::pt_t>(pts4, NPUPPI_MAX/8);
                //std::cout << " 5 FW :"; printArray<Puppi::pt_t>(pts5, NPUPPI_MAX/8);
                //std::cout << " 6 FW :"; printArray<Puppi::pt_t>(pts6, NPUPPI_MAX/8);
                //std::cout << " 7 FW :"; printArray<Puppi::pt_t>(pts7, NPUPPI_MAX/8);
                //std::cout << " 8 FW :"; printArray<Puppi::pt_t>(pts8, NPUPPI_MAX/8);
            }
            else if (DUT == 33)
            {
                std::cout << "- Ordered5:" << std::endl;
                std::cout << " FW :"; printArray<Puppi>(ordered_fw, NPUPPI_MAX);
                //std::cout << " REF:"; printArray<Puppi>(ordered_ref, NPUPPI_MAX);
            }
            else if (DUT == 35)
            {
                std::cout << "- Ordered7:" << std::endl;
                std::cout << "  1 FW :"; printArray<Puppi>(ordered_fw1  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref1 , NSPLITS);
                std::cout << "  2 FW :"; printArray<Puppi>(ordered_fw2  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref2 , NSPLITS);
                std::cout << "  3 FW :"; printArray<Puppi>(ordered_fw3  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref3 , NSPLITS);
                std::cout << "  4 FW :"; printArray<Puppi>(ordered_fw4  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref4 , NSPLITS);
                std::cout << "  5 FW :"; printArray<Puppi>(ordered_fw5  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref5 , NSPLITS);
                std::cout << "  6 FW :"; printArray<Puppi>(ordered_fw6  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref6 , NSPLITS);
                std::cout << "  7 FW :"; printArray<Puppi>(ordered_fw7  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref7 , NSPLITS);
                std::cout << "  8 FW :"; printArray<Puppi>(ordered_fw8  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref8 , NSPLITS);
                std::cout << "  9 FW :"; printArray<Puppi>(ordered_fw9  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref9 , NSPLITS);
                std::cout << " 10 FW :"; printArray<Puppi>(ordered_fw10 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref10, NSPLITS);
                std::cout << " 11 FW :"; printArray<Puppi>(ordered_fw11 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref11, NSPLITS);
                std::cout << " 12 FW :"; printArray<Puppi>(ordered_fw12 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref12, NSPLITS);
                std::cout << " 13 FW :"; printArray<Puppi>(ordered_fw13 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref13, NSPLITS);
                std::cout << " 14 FW :"; printArray<Puppi>(ordered_fw14 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref14, NSPLITS);
                std::cout << " 15 FW :"; printArray<Puppi>(ordered_fw15 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref15, NSPLITS);
                std::cout << " 16 FW :"; printArray<Puppi>(ordered_fw16 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref16, NSPLITS);
            }
            else if (DUT == 36)
            {
                std::cout << "- Ordered8:" << std::endl;
                std::cout << "  1 FW :"; printArray<Puppi>(ordered_fw1  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref1 , NSPLITS);
                std::cout << "  2 FW :"; printArray<Puppi>(ordered_fw2  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref2 , NSPLITS);
                std::cout << "  3 FW :"; printArray<Puppi>(ordered_fw3  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref3 , NSPLITS);
                std::cout << "  4 FW :"; printArray<Puppi>(ordered_fw4  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref4 , NSPLITS);
                std::cout << "  5 FW :"; printArray<Puppi>(ordered_fw5  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref5 , NSPLITS);
                std::cout << "  6 FW :"; printArray<Puppi>(ordered_fw6  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref6 , NSPLITS);
                std::cout << "  7 FW :"; printArray<Puppi>(ordered_fw7  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref7 , NSPLITS);
                std::cout << "  8 FW :"; printArray<Puppi>(ordered_fw8  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref8 , NSPLITS);
                std::cout << "  9 FW :"; printArray<Puppi>(ordered_fw9  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref9 , NSPLITS);
                std::cout << " 10 FW :"; printArray<Puppi>(ordered_fw10 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref10, NSPLITS);
                std::cout << " 11 FW :"; printArray<Puppi>(ordered_fw11 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref11, NSPLITS);
                std::cout << " 12 FW :"; printArray<Puppi>(ordered_fw12 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref12, NSPLITS);
                std::cout << " 13 FW :"; printArray<Puppi>(ordered_fw13 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref13, NSPLITS);
                std::cout << " 14 FW :"; printArray<Puppi>(ordered_fw14 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref14, NSPLITS);
                std::cout << " 15 FW :"; printArray<Puppi>(ordered_fw15 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref15, NSPLITS);
                std::cout << " 16 FW :"; printArray<Puppi>(ordered_fw16 , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref16, NSPLITS);
            }
            else if (DUT == 37)
            {
                std::cout << "- Ordered7bis:" << std::endl;
                std::cout << "  1 FW :"; printArray<Puppi>(ordered2_fw[0] , NSPLITS);
                std::cout << "  1 FW :"; printArray<Puppi>(ordered_fw1  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref1 , NSPLITS);
                std::cout << "  2 FW :"; printArray<Puppi>(ordered_fw2  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref2 , NSPLITS);
                std::cout << "  3 FW :"; printArray<Puppi>(ordered_fw3  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref3 , NSPLITS);
                std::cout << "  4 FW :"; printArray<Puppi>(ordered_fw4  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref4 , NSPLITS);
                std::cout << "  5 FW :"; printArray<Puppi>(ordered_fw5  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref5 , NSPLITS);
                std::cout << "  6 FW :"; printArray<Puppi>(ordered_fw6  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref6 , NSPLITS);
                std::cout << "  7 FW :"; printArray<Puppi>(ordered_fw7  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref7 , NSPLITS);
                std::cout << "  8 FW :"; printArray<Puppi>(ordered_fw8  , NSPLITS);
                std::cout << "    REF:"; printArray<Puppi>(ordered_ref8 , NSPLITS);
            }
            else if (DUT == 4 || DUT == 41)
            {
                std::cout << "- Merger:" << std::endl;
                std::cout << " FW :"; printArray<Puppi>(merged_fw , NPUPPI_MAX);
                std::cout << " REF:"; printArray<Puppi>(merged_ref, NPUPPI_MAX);
            }
            else if (DUT == 5)
            {
                std::cout << "- First triplet inputs:" << std::endl;
                std::cout << " FW : "; printArray<w3p_bdt::input_t>(inputs_fw , w3p_bdt::n_features);
                std::cout << " REF: "; printArray<w3p_bdt::input_t>(inputs_ref, w3p_bdt::n_features);
            }
            else if (DUT == 51)
            {
                std::cout << "- Event inputs:" << std::endl;
                std::cout << " FW : "; printArray<w3p_bdt::input_t>(BDT_inputs_fw[0] , w3p_bdt::n_features);
                std::cout << " REF: "; printArray<w3p_bdt::input_t>(BDT_inputs_ref[0], w3p_bdt::n_features);
            }
            else if (DUT == 52)
            {
                std::cout << "- Get Cos Phi:" << std::endl;
                Puppi::phi_t myphi = selected_fw[0].hwPhi;
                Puppi::eta_t myeta = selected_fw[0].hwEta;
                std::cout << " - Phi    : " << myphi   << " -> toFLoat: " << Puppi::floatPhi(myphi) << std::endl;
                std::cout << "   cosphi : " << cosphi  << " -> toFloat: " << cosphi/512.  << " / std::cos : " << std::cos(Puppi::floatPhi(myphi))  << std::endl;
                std::cout << " - Eta    : " << myeta   << " -> toFLoat: " << Puppi::floatEta(myeta) << std::endl;
                std::cout << "   cosheta: " << cosheta << " -> toFloat: " << cosheta/10. << " / std::cosh: " << std::cosh(Puppi::floatEta(myeta)) << std::endl;
            }
            else if (DUT == 53)
            {
                std::cout << "- Inv mass:" << std::endl;
                std::cout << " FW : " << mass_fw << std::endl;
                std::cout << " REF: " << mass_ref << std::endl;
            }
            else if (DUT == 6)
            {
                std::cout << "- BDT scores:" << std::endl;
                std::cout << " FW : "; printArray<w3p_bdt::score_t>(BDT_scores_fw , NTRIPLETS);
                std::cout << " REF: "; printArray<w3p_bdt::score_t>(BDT_scores_ref, NTRIPLETS);
            }
            else if (DUT == 7)
            {
                std::cout << " - BDT scores:" << std::endl;
                std::cout << " FW : "; printArray<w3p_bdt::score_t>(BDT_scores_fw , NTRIPLETS);
                std::cout << " REF: "; printArray<w3p_bdt::score_t>(BDT_scores_ref, NTRIPLETS);
                std::cout << "- BDT highest score:" << std::endl;
                std::cout << " FW : " << max_score_fw << std::endl;
                std::cout << " REF: " << max_score_ref << std::endl;
            }
            else if (DUT == 10 || DUT == 11 || DUT == 12)
            {
                std::cout << "- EventProcessor:" << std::endl;
                std::cout << " - BDT scores:" << std::endl;
                std::cout << "  FW : "; printArray<w3p_bdt::score_t>(BDT_scores_fw , NTRIPLETS);
                std::cout << "  REF: "; printArray<w3p_bdt::score_t>(BDT_scores_ref, NTRIPLETS);
                std::cout << " - Max score:" << std::endl;
                std::cout << "  FW : " << max_score_fw << std::endl;
                std::cout << "  REF: " << max_score_ref << std::endl;
            }
        }

        // - Test FW vs REF -
        if (DUT == 1)
        {
            for (unsigned int i=0; i<NPUPPI_MAX; i++)
            {
                if (masked_fw[i] != masked_ref[i])
                {
                    std::cout << "---> Different mask at i: " << i << " -> FW: " << masked_fw[i] << " REF: " << masked_ref[i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 2)
        {
            //for (unsigned int i=0; i<NPUPPI_SEL; i++)
            //{
            //    if (slimmed_fw[i] != slimmed_ref[i])
            //    {
            //        std::cout << "---> Different slimmed at i: " << i << " -> FW: " << slimmed_fw[i] << " REF: " << slimmed_ref[i] << std::endl;
            //        return 1;
            //    }
            //}
            for (unsigned int i=0; i<NPUPPI_MAX; i++)
            {
                if (slimmed_fw[i] != slimmed_ref[i])
                {
                    std::cout << "---> Different slimmed at i: " << i << " -> FW: " << slimmed_fw[i] << " REF: " << slimmed_ref[i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 3)
        {
            for (unsigned int i=0; i<NPUPPI_MAX; i++)
            {
                if (ordered_idxs_fw[i] != ordered_idxs_ref[i])
                {
                    std::cout << "---> Different idx at i: " << i << " -> FW: " << ordered_idxs_fw[i] << " REF: " << ordered_idxs_ref[i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 31)
        {
            for (unsigned int i=0; i<NPUPPI_MAX/4; i++)
            {
                if (ordered_idxs_fw1[i] != ordered_idxs_ref1[i])
                {
                    std::cout << "---> Different idx1 at i: " << i << " -> FW: " << ordered_idxs_fw1[i] << " REF: " << ordered_idxs_ref1[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw2[i] != ordered_idxs_ref2[i])
                {
                    std::cout << "---> Different idx2 at i: " << i << " -> FW: " << ordered_idxs_fw2[i] << " REF: " << ordered_idxs_ref2[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw3[i] != ordered_idxs_ref3[i])
                {
                    std::cout << "---> Different idx3 at i: " << i << " -> FW: " << ordered_idxs_fw3[i] << " REF: " << ordered_idxs_ref3[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw4[i] != ordered_idxs_ref4[i])
                {
                    std::cout << "---> Different idx4 at i: " << i << " -> FW: " << ordered_idxs_fw4[i] << " REF: " << ordered_idxs_ref4[i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 32)
        {
            for (unsigned int i=0; i<NPUPPI_MAX/8; i++)
            {
                if (ordered_idxs_fw1[i] != ordered_idxs_ref1[i])
                {
                    std::cout << "---> Different idx1 at i: " << i << " -> FW: " << ordered_idxs_fw1[i] << " REF: " << ordered_idxs_ref1[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw2[i] != ordered_idxs_ref2[i])
                {
                    std::cout << "---> Different idx2 at i: " << i << " -> FW: " << ordered_idxs_fw2[i] << " REF: " << ordered_idxs_ref2[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw3[i] != ordered_idxs_ref3[i])
                {
                    std::cout << "---> Different idx3 at i: " << i << " -> FW: " << ordered_idxs_fw3[i] << " REF: " << ordered_idxs_ref3[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw4[i] != ordered_idxs_ref4[i])
                {
                    std::cout << "---> Different idx4 at i: " << i << " -> FW: " << ordered_idxs_fw4[i] << " REF: " << ordered_idxs_ref4[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw5[i] != ordered_idxs_ref5[i])
                {
                    std::cout << "---> Different idx5 at i: " << i << " -> FW: " << ordered_idxs_fw5[i] << " REF: " << ordered_idxs_ref5[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw6[i] != ordered_idxs_ref6[i])
                {
                    std::cout << "---> Different idx6 at i: " << i << " -> FW: " << ordered_idxs_fw6[i] << " REF: " << ordered_idxs_ref6[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw7[i] != ordered_idxs_ref7[i])
                {
                    std::cout << "---> Different idx7 at i: " << i << " -> FW: " << ordered_idxs_fw7[i] << " REF: " << ordered_idxs_ref7[i] << std::endl;
                    return 1;
                }
                if (ordered_idxs_fw8[i] != ordered_idxs_ref8[i])
                {
                    std::cout << "---> Different idx8 at i: " << i << " -> FW: " << ordered_idxs_fw8[i] << " REF: " << ordered_idxs_ref8[i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 35 || DUT == 36 /*|| DUT == 37*/)
        {
            for (unsigned int i=0; i<NPUPPI_MAX/8; i++)
            {
                if (ordered_fw1[i] != ordered_ref1[i])
                {
                    std::cout << "---> Different ordered1 at i: " << i << " -> FW: " << ordered_fw1[i] << " REF: " << ordered_ref1[i] << std::endl;
                    return 1;
                }
                if (ordered_fw2[i] != ordered_ref2[i])
                {
                    std::cout << "---> Different ordered2 at i: " << i << " -> FW: " << ordered_fw2[i] << " REF: " << ordered_ref2[i] << std::endl;
                    return 1;
                }
                if (ordered_fw3[i] != ordered_ref3[i])
                {
                    std::cout << "---> Different ordered3 at i: " << i << " -> FW: " << ordered_fw3[i] << " REF: " << ordered_ref3[i] << std::endl;
                    return 1;
                }
                if (ordered_fw4[i] != ordered_ref4[i])
                {
                    std::cout << "---> Different ordered4 at i: " << i << " -> FW: " << ordered_fw4[i] << " REF: " << ordered_ref4[i] << std::endl;
                    return 1;
                }
                if (ordered_fw5[i] != ordered_ref5[i])
                {
                    std::cout << "---> Different ordered5 at i: " << i << " -> FW: " << ordered_fw5[i] << " REF: " << ordered_ref5[i] << std::endl;
                    return 1;
                }
                if (ordered_fw6[i] != ordered_ref6[i])
                {
                    std::cout << "---> Different ordered6 at i: " << i << " -> FW: " << ordered_fw6[i] << " REF: " << ordered_ref6[i] << std::endl;
                    return 1;
                }
                if (ordered_fw7[i] != ordered_ref7[i])
                {
                    std::cout << "---> Different ordered7 at i: " << i << " -> FW: " << ordered_fw7[i] << " REF: " << ordered_ref7[i] << std::endl;
                    return 1;
                }
                if (ordered_fw8[i] != ordered_ref8[i])
                {
                    std::cout << "---> Different ordered8 at i: " << i << " -> FW: " << ordered_fw8[i] << " REF: " << ordered_ref8[i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 4 || DUT == 41)
        {
            for (unsigned int i=0; i<NPUPPI_MAX; i++)
            {
                if (merged_fw[i] != merged_ref[i])
                {
                    std::cout << "---> Different idx at i: " << i << " -> FW: " << merged_fw[i] << " REF: " << merged_ref[i] << std::endl;
                    //return 1; // FIXME: uncomment when ordering of same pT candidates in FW is fixed
                }
            }
        }
        else if (DUT == 5)
        {
            for (unsigned int i = 0; i < w3p_bdt::n_features; i++)
            {
                if (inputs_fw[i] != inputs_ref[i])
                {
                    std::cout << "---> Different BDT input at i: " << i << " -> FW: " << inputs_fw[i] << " REF: " << inputs_ref[i] << std::endl;
                    //return 1; // FIXME: uncomment when ordering of same pT candidates in FW is fixed
                }
            }
        }
        else if (DUT == 51)
        {
            for (unsigned int i = 0; i < w3p_bdt::n_features; i++)
            {
                if (BDT_inputs_fw[0][i] != BDT_inputs_ref[0][i])
                {
                    std::cout << "---> Different Event input at i: " << i << " -> FW: " << BDT_inputs_fw[0][i] << " REF: " << BDT_inputs_ref[0][i] << std::endl;
                    return 1;
                }
            }
        }
        else if (DUT == 6)
        {
            for (unsigned int i = 0; i < NTRIPLETS; i++)
            {
                if (BDT_scores_fw[i] != BDT_scores_ref[i])
                {
                    std::cout << "---> Different BDT score at i: " << i << " -> FW: " << BDT_scores_fw[i] << " REF: " << BDT_scores_ref[i] << std::endl;
                    //return 1; // FIXME: uncomment when ordering of same pT candidates in FW is fixed
                }
            }
        }
        else if (DUT == 7)
        {
            if (max_score_fw != max_score_ref)
            {
                std::cout << "---> Different highest BDT score -> FW: " << max_score_fw << " REF: " << max_score_ref << std::endl;
                return 1;
            }
        }
        else if (DUT == 10 || DUT == 11 || DUT == 12)
        {
            if (max_score_fw != max_score_ref)
            {
                std::cout << "---> EP Different -> FW: " << max_score_fw << " REF: " << max_score_ref << std::endl;
                return 1;
            }
        }

    } // end loop on data chunks
    return 0;
}
