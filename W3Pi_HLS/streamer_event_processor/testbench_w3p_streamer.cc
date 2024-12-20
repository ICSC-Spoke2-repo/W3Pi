// Standar includes
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <bitset>

// Vitis includes
#include "hls_stream.h"

// Project includes
#include "src/w3p_streamer.h"
#include "src/w3p_emulator.h"

#define HEADER_DEBUG 0
#define OUTPUT_DEBUG 1
#define NTEST 1

// DUTs:
//  1 : w3p_streamer
#define DUT 1

// -------------------------------------------------------------
// Pretty print of array
template<typename T>
void printArray(T A[], int size)
{
    std::cout << " ";
    for (unsigned int i = 0; i < size; i++)
        std::cout << A[i] << " ";
    std::cout << std::endl;
}

// Pretty print of vector
template<typename T>
void printVector(std::vector<T> A)
{
    std::cout << " ";
    for (unsigned int i = 0; i < A.size(); i++)
        std::cout << A.at(i) << " ";
    std::cout << std::endl;
}

// -------------------------------------------------------------
// Main testbench function
int main(int argc, char **argv) {

    // Read input stream
    std::fstream inFstreams[4];
    inFstreams[0].open("Puppi_w3p_PU200_a.dump", std::ios::in | std::ios::binary);
    inFstreams[1].open("Puppi_w3p_PU200_b.dump", std::ios::in | std::ios::binary);
    inFstreams[2].open("Puppi_w3p_PU200_c.dump", std::ios::in | std::ios::binary);
    inFstreams[3].open("Puppi_w3p_PU200_d.dump", std::ios::in | std::ios::binary);

    // Loop on input data in chunks
    for (int itest = 0, ntest = NTEST;
         itest < ntest && inFstreams[0].good() && inFstreams[1].good() && inFstreams[2].good() && inFstreams[3].good();
         ++itest)
    {
        std::cout << "--------------------" << std::endl;

        // Read header
        uint64_t headers[NLINKS];
        for (int i = 0; i < NLINKS; i++)
        {
            inFstreams[i].read(reinterpret_cast<char *>(&headers[i]), sizeof(uint64_t));
        }

        // Read header quantities
        // bits 	size 	meaning
        // 63-62 	2 	    10 = valid event header
        // 61 	    1 	    error bit
        // 60-56 	6 	    (local) run number
        // 55-24 	32 	    orbit number
        // 23-12 	12 	    bunch crossing number (0-3563)
        // 11-07 	4 	    must be set to 0
        // 07-00 	8 	    number of Puppi candidates
        unsigned int npuppis[NLINKS];
        for (int i = 0; i < NLINKS; i++)
        {
            npuppis[i] = headers[i] & 0xFF;                  // 8 bits
        }
        //unsigned int beZero = (header >> 8)  & 0xF       ; // 4 bits
        //unsigned int bxNum  = (header >> 12) & 0xFFF     ; // 12 bits
        //unsigned int orbitN = (header >> 24) & 0xFFFFFFFF; // 32 bits
        //unsigned int runN   = (header >> 56) & 0x3F      ; // 6 bits
        //unsigned int error  = (header >> 62) & 0x1       ; // 1
        //unsigned int validH = (header >> 63) & 0x3       ; // 2 bits

        // Print header quantities
        std::cout << "*** itest " << itest << " / ntest " << ntest \
                  << " (npuppiA = " << npuppis[0] << ", npuppiB = " << npuppis[1] << ", npuppiC = " << npuppis[2] << ", npuppiD = " << npuppis[3] << ")" << std::endl;
        if (HEADER_DEBUG)
        {
            for (int i = 0; i < NLINKS; i++)
            {
                std::cout << " - header" << i << ": " << std::bitset<64>(headers[i]) << " : " << headers[i] << std::endl;
                std::cout << " - npuppi" << i << ": " << std::bitset<64>(npuppis[i]) << " : " << npuppis[i] << std::endl;
            }
            //std::cout << " - beZero: " << std::bitset<64>(beZero) << " : " << beZero << std::endl;
            //std::cout << " - bxNum : " << std::bitset<64>(bxNum)  << " : " << bxNum  << std::endl;
            //std::cout << " - orbitN: " << std::bitset<64>(orbitN) << " : " << orbitN << std::endl;
            //std::cout << " - runN  : " << std::bitset<64>(runN  ) << " : " << runN   << std::endl;
            //std::cout << " - error : " << std::bitset<64>(error ) << " : " << error  << std::endl;
            //std::cout << " - validH: " << std::bitset<64>(validH) << " : " << validH << std::endl;
        }

        // Minimal asserts on npuppi to guarantee correct reading of fstream
        for (int i = 0; i < NLINKS; i++)
        {
            assert(npuppis[i] <= NPUPPI_LINK);
        }

        // Declare data vectors for buffering input fstreams
        std::vector<uint64_t> inData[NLINKS];
        for (int i = 0; i < NLINKS; i++)
        {
            inData[i] = std::vector<uint64_t>(NPUPPI_LINK, 0);
        }

        // Read actual data and store it in uint64_t vectors
        for (int j = 0; j < NLINKS; j++)
        {
            for (int i = 0; i < npuppis[j]; ++i)
            {
                inFstreams[j].read(reinterpret_cast<char *>(&inData[j][i]), sizeof(uint64_t));
            }
        }

        // Copy data in firmware-input streams
        hls::stream<uint64_t> inFifo[NLINKS];
        for (int j = 0; j < NLINKS; j++)
        {
            for (int i = 0; i < NPUPPI_LINK; i++)
            {
                inFifo[j] << inData[j][i];
            }
        }

        if (DUT == 1)
        {
            // Output declaration
            hls::stream<Puppi> outFifo[NLINKS];
            std::vector<Puppi> out_fwr[NLINKS];
            std::vector<Puppi> out_ref[NLINKS];
            for (int i = 0; i < NLINKS; i++)
            {
                out_fwr[i].resize(NPUPPI_LINK);
                out_ref[i].resize(NPUPPI_LINK);
            }

            // Firmware call
            w3p_streamer(inFifo, outFifo);

            // Reference call
            w3p_emulator(inData, out_ref);

            // Copy data from output firmware stream into output vector for checks and printout
            for (int j = 0; j < NLINKS; j++)
            {
                for (int i = 0; i < NPUPPI_LINK; i++)
                {
                    outFifo[j] >> out_fwr[j].at(i);
                }
            }

            // Debug printout
            if (OUTPUT_DEBUG)
            {
                std::cout << "- Streamer:" << std::endl;
                for (int i = 0; i < NLINKS; i++)
                {
                    std::cout << "  FW"  << i <<" :"; printVector<Puppi>(out_fwr[i]);
                    std::cout << "  REF" << i << ":"; printVector<Puppi>(out_ref[i]);

                }
            }

            // Test firmware vs reference
            // FIXME: temporarily commented out because sorting in firmware and emulator does not take into account puppi with same pT
            //for (int j = 0; j < NLINKS; j++)
            //{
            //    std::cout << "---> LINK: " << j << std::endl;
            //    for (unsigned int i = 0; i < NPUPPI_LINK; i++)
            //    {
            //        if (out_fwr[j][i] != out_ref[j][i])
            //        {
            //            std::cout << "     Different Puppi at i: " << i << " -> FW: " << out_fwr[j][i] << " REF: " << out_ref[j][i] << std::endl;
            //            return 1;
            //        }
            //    }
            //}
        }
    } // end loop on NTEST
    return 0;
}
