#include "data.h"
#include "bitonic_hybrid.h"

//---------------------------------------------------------
// Read input stream and decode uint64_t into Pupppi candidate
inline void decoder (hls::stream<uint64_t> &inFifo, hls::stream<Puppi> &outFifo)
{
    // Output Puppi
    Puppi tmpPuppi;

    // Read and decode
    LOOP_DECODER: for (size_t i = 0; i < NPUPPI_LINK; i++)
    {
        #pragma HLS pipeline

        // Read input uint64_t
        uint64_t tmpIn = inFifo.read();

        // Unpack data into Puppi
        tmpPuppi.unpack(tmpIn);

        // Return output Puppi
        outFifo << tmpPuppi;
    }
}

// ------------------------------------------------------------------
// Masker method (apply selections)
void masker (hls::stream<Puppi> &inPuppi, hls::stream<Puppi> &maskedPuppi)
{
    // Dummy Puppi candidate
    Puppi dummyPuppi;
    dummyPuppi.clear();

    // Output masked Puppi
    Puppi outPuppi;

    // Read and apply selections
    LOOP_MASKER: for (size_t i = 0; i < NPUPPI_LINK; i++)
    {
        #pragma HLS pipeline

        // Read input Puppi
        Puppi tmpPuppi = inPuppi.read();

        // Apply selections
        //bool badPt  = (tmpPuppi.hwPt < 3.25 );
        bool badEta = (tmpPuppi.hwEta < -Puppi::ETA_CUT || tmpPuppi.hwEta > Puppi::ETA_CUT);
        bool badID  = (tmpPuppi.hwID < 2 || tmpPuppi.hwID > 5);
        bool masked = (badEta || badID);

        // Fill output Puppi
        outPuppi = masked ? dummyPuppi : tmpPuppi;

        // Copy output puppi to output stream
        maskedPuppi << outPuppi;
    }
}

//---------------------------------------------------------
// Merging methods
// get_bitonic_sequence
void get_bitonic_sequenceA(const Puppi in1[NSORTING], const Puppi in2[NSORTING], Puppi bitonic[2*NSORTING])
{
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=bitonic complete
    LOOP_MAKE_BITONIC: for (int id = 0, ia=NSORTING-1; id<NSORTING; id++, ia--)
    {
        #pragma HLS UNROLL
        bitonic[id] = in1[ia];
        bitonic[id+(NSORTING)] = in2[id];
    }
}

// merge_sort
void merge_sortA(const Puppi in1[NSORTING], const Puppi in2[NSORTING], Puppi sorted_out[2*NSORTING])
{
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=sorted_out complete
    get_bitonic_sequenceA(in1, in2, sorted_out);
    hybridBitonicSort::bitonicMerger<Puppi, 2*NSORTING, 0>::run(sorted_out, 0);
}

//---------------------------------------------------------
// Sort Puppi candidates
void sorter (hls::stream<Puppi> &inPuppi, Puppi sortedPuppi[NPUPPI_LINK])
{
    // Buffer to read input Puppi
    Puppi accumulatedPuppi[NCHUNKS][NSORTING];

    // Fill buffer with first half of input Puppi
    LOOP_SORTER_HALF1: for (unsigned int i = 0; i < NSORTING; ++i)
    {
        #pragma HLS pipeline
        accumulatedPuppi[0][i] = inPuppi.read();
    }

    // Sort first half and fill second half in parallel
    LOOP_SORTER_CHUNKS: for (unsigned int i = 1; i < NCHUNKS; ++i)
    {
        // Sort first half
        hybridBitonicSort::bitonicSorter<Puppi, NSORTING, 0, true>::run(accumulatedPuppi[0], 0);

        // Fill second half
        LOOP_SORTER_HALF2: for (unsigned int j = 0; j < NSORTING; ++j) {
            #pragma HLS pipeline
            accumulatedPuppi[i][j] = inPuppi.read();
        }
    }

    // Sort second half
    hybridBitonicSort::bitonicSorter<Puppi, NSORTING, 0, true>::run(accumulatedPuppi[1], 0);

    // Merge sorted halves
    merge_sortA(accumulatedPuppi[0], accumulatedPuppi[1], sortedPuppi);
}

//---------------------------------------------------------
// Write to output stream
void writer (Puppi sortedPuppi[NPUPPI_LINK], hls::stream<Puppi> &outFifo)
{
    LOOP_WRITER: for (size_t i = 0; i < NPUPPI_LINK; i++)
    {
        #pragma HLS pipeline
        outFifo << sortedPuppi[i];
    }
}

//---------------------------------------------------------
// Top function
void w3p_streamer (hls::stream<uint64_t> inFifo[NLINKS], hls::stream<Puppi> outFifo[NLINKS])
{
    #pragma HLS DATAFLOW

    // Utility streams
    // Using depth=NPUPPI_LINK you gain one clock in latency, but loose a bit of resources
    // #pragma HLS stream variable=decoded_stream depth=NPUPPI_LINK
    // #pragma HLS stream variable=masked_stream  depth=NPUPPI_LINK
    hls::stream<Puppi> decoded_stream[NLINKS];
    hls::stream<Puppi> masked_stream[NLINKS];

    // Array of sorted Puppi candidates
    // This array is automatically partitioned as:
    // #pragma HLS ARRAY_PARTITION variable=sortedPuppi type=complete dim=2
    Puppi sortedPuppi[NLINKS][NPUPPI_LINK];

    // Loop on input streams and process data
    LOOP_NLINKS: for (int i = 0; i < NLINKS; i++)
    {
        #pragma HLS UNROLL

        // Actual call to sub-routines
        decoder(inFifo[i], decoded_stream[i]);
        masker (decoded_stream[i], masked_stream[i]);
        sorter (masked_stream[i], sortedPuppi[i]);

        // Copy to output stream
        writer(sortedPuppi[i], outFifo[i]);
    }
}