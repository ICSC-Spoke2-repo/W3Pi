#include "src/event_preparator.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <bitset>

int main(int argc, char **argv) {

    // Variables to read data
    uint64_t header, data[NPUPPI_MAX];

    // Read input stream
    //std::fstream in("Puppi.dump", std::ios::in | std::ios::binary);
    std::fstream in("Puppi_w3p_PU200.dump", std::ios::in | std::ios::binary);

    // Loop on input data in chunks
    for (int itest = 0, ntest = 10; itest < ntest && in.good(); ++itest) {

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
        if (DEBUG)
        {
            std::cout << "*** itest " << itest << " / ntest " << ntest << std::endl;
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
        if (npuppi < 3) continue; // FIXME: this should actually be moved to the firmwere...how??

        // Declare data variables
        Puppi puppi[NPUPPI_MAX], output[NPUPPI_MAX];
        Puppi::pt_t output_absiso[NPUPPI_MAX];

        // Loop on puppi candidates
        if (itest == 0 && DEBUG) std::cout << " - Particles:" << std::endl;
        for (unsigned int i = 0; i < npuppi; ++i)
        {
            // Unpack data into puppi candidates
            puppi[i].unpack(data[i]);

            // Debug printouts of puppi candidates
            if (itest == 0 && DEBUG)
            {
                printf("  %3u/%3u : pT %8.3f eta %+6.3f phi %+6.3f pid %1u\n",
                        i, npuppi, puppi[i].floatPt(), puppi[i].floatEta(),
                        puppi[i].floatPhi(), puppi[i].hwID.to_uint());
                if (i == 0 && DEBUG)
                {
                    ap_uint<64> casted = ap_uint<64>(data[i]);
                    std::cout << "    |_ casted: " << std::bitset<64>(casted) << std::endl;
                    std::cout << "    |_ pT    : " << std::bitset<14>(casted(13,0)) << std::endl;
                    std::cout << "    |_ eta   : " << std::bitset<12>(casted(25,14)) << std::endl;
                    std::cout << "    |_ phi   : " << std::bitset<11>(casted(36,26)) << std::endl;
                    std::cout << "    |_ PID   : " << std::bitset<3>(casted(39,37)) << std::endl;
                    std::cout << "    |_ rest  : " << std::bitset<24>(casted(63,40)) << std::endl;
                }
            }
        }
        // Clear remaining particles in array
        for (unsigned int i = npuppi; i < NPUPPI_MAX; ++i)
        {
            puppi[i].clear();
        }

        // Here do the processing
        //  - HLS kernel
        //  - c++ code
        //  - compare the two

        // FIRMWARE call
        Puppi pivot_hls;
        event_preparator(puppi, pivot_hls);

        // REFERENCE call
        Puppi pivot_cpp;
        event_preparator_ref(npuppi, puppi, pivot_cpp);

        // COMPARE
        bool ok = true;
        ok = ok && (pivot_hls.pack() == pivot_cpp.pack());
        if (!ok) {
            printf("Mismatch in test %u!\n", itest);
            std::cout << " - Pivot HLS (pt/eta/phi/ID): " << pivot_hls.floatPt() << "/" << pivot_hls.floatEta() << "/" << pivot_hls.floatPhi()
                                                       << "/" << pivot_hls.hwID.to_int() << std::endl;
            std::cout << " - Pivot C++ (pt/eta/phi/ID): " << pivot_cpp.floatPt() << "/" << pivot_cpp.floatEta() << "/" << pivot_cpp.floatPhi()
                                                       << "/" << pivot_cpp.hwID.to_int() << std::endl;
            return 1;
        } else {
            printf("Test %u passed\n", itest);
        }
    }
    return 0;
}
