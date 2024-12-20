#include "w3p_emulator.h"
#include <algorithm>

#define DEBUG 0
#define DEEP_DEBUG 0

// ------------------------------------------------------------------
bool puppiComparator (Puppi a, Puppi b)
{
    return (a.hwPt > b.hwPt);
}

// ------------------------------------------------------------------
void w3p_emulator(const std::vector<uint64_t> input_stream[NLINKS], std::vector<Puppi> output_stream[NLINKS])
{

    for (int nfifo = 0; nfifo < NLINKS; nfifo++)
    {
        // Unpack and mask
        for (int i = 0; i < NPUPPI_LINK; ++i)
        {
            // Unpack data into puppi candidates
            Puppi unpackedPuppi;
            unpackedPuppi.unpack(input_stream[nfifo][i]);

            // Dummy puppi candidate
            Puppi dummy;
            dummy.clear();

            // Apply selections
            bool badEta = ( std::abs(unpackedPuppi.hwEta) > Puppi::ETA_CUT );
            bool badID  = ( unpackedPuppi.hwID < 2 || unpackedPuppi.hwID > 5 );

            if (badEta || badID)
            {
                output_stream[nfifo].at(i) = dummy;
            }
            else
            {
                output_stream[nfifo].at(i) = unpackedPuppi;
            }

            // Debug printouts of puppi candidates
            if (DEBUG)
            {
                Puppi myoutput = output_stream[nfifo].at(i);
                printf("  %3u/%3u : pT %6.2f  eta %+6.3f  phi %+6.3f  pid %1u  Z0 %+6.3f\n",
                        i, NPUPPI_LINK, myoutput.floatPt(), myoutput.floatEta(),
                        myoutput.floatPhi(), myoutput.hwID.to_uint(), myoutput.floatZ0());
                if (DEEP_DEBUG)
                {
                    ap_uint<64> casted = ap_uint<64>(input_stream[nfifo][i]);
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

        // Sorting
        std::stable_sort(output_stream[nfifo].begin(), output_stream[nfifo].end(), puppiComparator);

    } // end loop on NLINKS
}


