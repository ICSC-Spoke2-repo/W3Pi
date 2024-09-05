#include "event_processor.h"
#include "bitonic_hybrid.h"

// ------------------------------------------------------------------
// Select L1Puppi objects
void masker (const Puppi input[NPUPPI_MAX], ap_uint<NPUPPI_MAX> & masked)
{
    #pragma HLS ARRAY_PARTITION variable=input complete

    // Clear masked array
    masked = 0;

    // Select
    LOOP_MASKER_FILL: for (unsigned int i = 0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        //bool badPt  = (input[i].hwPt < 3.25 );
        bool badEta = (input[i].hwEta < -Puppi::ETA_CUT || input[i].hwEta > Puppi::ETA_CUT);
        bool badID  = (input[i].hwID < 2 || input[i].hwID > 5);
        //masked[i] = (badPt || badEta || badID);
        masked[i] = (badEta || badID);
    }
}


// ------------------------------------------------------------------
// Slimmer
void slimmer (const ap_uint<NPUPPI_MAX> masked, idx_t slimmed_idxs[NPUPPI_SEL])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed_idxs complete

    LOOP_SLIMMER_CLEAR: for (int i = 0; i < NPUPPI_SEL; i++)
    {
        #pragma HLS UNROLL
        slimmed_idxs[i] = NPUPPI_SEL + 1;
    }

    unsigned int slim_idx = 0;
    LOOP_SLIMMER_FILL: for (unsigned int i = 0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        bool update = ( (slim_idx < NPUPPI_SEL) && !masked[i] );
        slimmed_idxs[slim_idx] = update ? idx_t(i) : slimmed_idxs[slim_idx];
        slim_idx = update ? slim_idx + 1 : slim_idx;

        //slimmed_idxs[slim_idx] = (!masked[i]) ? idx_t(i) : slimmed_idxs[slim_idx];
        //slim_idx = (!masked[i]) ? slim_idx + 1 : slim_idx;
    }
}

// Replace masked candidates with empty/dummy puppi
void slimmer2 (const Puppi input[NPUPPI_MAX], const ap_uint<NPUPPI_MAX> masked, Puppi slimmed[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=slimmed complete

    Puppi dummy;
    dummy.clear();

    LOOP_SLIMMER_FILL: for (unsigned int i = 0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        slimmed[i] = masked[i] ? dummy : input[i];
    }
}

// ------------------------------------------------------------------
// Order selected indexes
// bubbleSort algo from https://www.geeksforgeeks.org/bubble-sort-in-cpp/
void bubbleSort (Puppi::pt_t pts[NPUPPI_SEL], idx_t ordered_idxs[NPUPPI_SEL])
{
    #pragma HLS PIPELINE II=13

    LOOP_BS1: for (unsigned int i = 0; i < NPUPPI_SEL - 1; i++)
    {
        LOOP_BS2: for (unsigned int j = 0; j < NPUPPI_SEL - 1; j++)
        {
            if ( (j < NPUPPI_SEL-1-i) )
            {
                Puppi::pt_t tmp_pt = pts[j];
                idx_t tmp_idx = ordered_idxs[j];

                bool swap = (pts[j] < pts[j+1]);

                pts[j]   = swap ? pts[j+1] : tmp_pt;
                pts[j+1] = swap ? tmp_pt : pts[j+1] ;

                ordered_idxs[j]   = swap ? ordered_idxs[j+1] : tmp_idx;
                ordered_idxs[j+1] = swap ? tmp_idx : ordered_idxs[j+1];
            }
        }
    }
}

// Same as bubbleSort but from slimmer2
void bubbleSort2 (Puppi::pt_t pts[NPUPPI_MAX], idx_t ordered_idxs[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=pts complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    LOOP_BS1: for (unsigned int i = 0; i < NPUPPI_MAX - 1; i++)
    {
        LOOP_BS2: for (unsigned int j = 0; j < NPUPPI_MAX - 1; j++)
        {
            if ( (j < NPUPPI_MAX-1-i) )
            {
                Puppi::pt_t tmp_pt = pts[j];
                idx_t tmp_idx = ordered_idxs[j];

                bool swap = (pts[j] < pts[j+1]);

                pts[j]   = swap ? pts[j+1] : tmp_pt;
                pts[j+1] = swap ? tmp_pt : pts[j+1] ;

                ordered_idxs[j]   = swap ? ordered_idxs[j+1] : tmp_idx;
                ordered_idxs[j+1] = swap ? tmp_idx : ordered_idxs[j+1];
            }
        }
    }
}

// Same as bubbleSort2 but split into 4 arrays of 52 ( = 208/4) elements
void bubbleSort3 (Puppi::pt_t pts[NPUPPI_MAX/4], idx_t ordered_idxs[NPUPPI_MAX/4])
{
    #pragma HLS ARRAY_PARTITION variable=pts complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    LOOP_BS1: for (unsigned int i = 0; i < NPUPPI_MAX/4 - 1; i++)
    {
        LOOP_BS2: for (unsigned int j = 0; j < NPUPPI_MAX/4 - 1; j++)
        {
            if ( (j < NPUPPI_MAX/4 -1-i) )
            {
                Puppi::pt_t tmp_pt = pts[j];
                idx_t tmp_idx = ordered_idxs[j];

                bool swap = (pts[j] < pts[j+1]);

                pts[j]   = swap ? pts[j+1] : tmp_pt;
                pts[j+1] = swap ? tmp_pt : pts[j+1] ;

                ordered_idxs[j]   = swap ? ordered_idxs[j+1] : tmp_idx;
                ordered_idxs[j+1] = swap ? tmp_idx : ordered_idxs[j+1];
            }
        }
    }
}

// Same as bubbleSort2 but split into 8 arrays of 26 ( = 208/8) elements
void bubbleSort4 (Puppi::pt_t pts[NPUPPI_MAX/8], idx_t ordered_idxs[NPUPPI_MAX/8])
{
    #pragma HLS ARRAY_PARTITION variable=pts complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    LOOP_BS1: for (unsigned int i = 0; i < NPUPPI_MAX/8 - 1; i++)
    {
        LOOP_BS2: for (unsigned int j = 0; j < NPUPPI_MAX/8 - 1; j++)
        {
            if ( (j < NPUPPI_MAX/8 -1-i) )
            {
                Puppi::pt_t tmp_pt = pts[j];
                idx_t tmp_idx = ordered_idxs[j];

                bool swap = (pts[j] < pts[j+1]);

                pts[j]   = swap ? pts[j+1] : tmp_pt;
                pts[j+1] = swap ? tmp_pt : pts[j+1] ;

                ordered_idxs[j]   = swap ? ordered_idxs[j+1] : tmp_idx;
                ordered_idxs[j+1] = swap ? tmp_idx : ordered_idxs[j+1];
            }
        }
    }
}

// Same as bubbleSort2 but split into 16 arrays of 13 ( = 208/16) elements
void bubbleSort8 (Puppi ordered[NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=ordered complete

    LOOP_BS1: for (unsigned int i = 0; i < NSPLITS - 1; i++)
    {
        LOOP_BS2: for (unsigned int j = 0; j < NSPLITS - 1; j++)
        {
            if ( (j < NSPLITS -1-i) )
            {
                Puppi tmp_puppi = ordered[j];
                bool swap = (ordered[j].hwPt < ordered[j+1].hwPt);
                ordered[j]   = swap ? ordered[j+1] : tmp_puppi;
                ordered[j+1] = swap ? tmp_puppi : ordered[j+1];
            }
        }
    }
}

// Order idxs by pT
void orderer (const Puppi input[NPUPPI_MAX], const idx_t slimmed_idxs[NPUPPI_SEL], idx_t ordered_idxs[NPUPPI_SEL])
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=slimmed_idxs complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    Puppi::pt_t pts[NPUPPI_SEL];
    #pragma HLS ARRAY_PARTITION variable=pts complete

    // Pre-ordering pTs and idxs
    LOOP_ORDERER_PRE: for (unsigned int i=0; i < NPUPPI_SEL; i++)
    {
        #pragma HLS UNROLL
        pts[i] = input[ slimmed_idxs[i] ].hwPt;
        ordered_idxs[i] = slimmed_idxs[i];
    }

    // Sort pts and indexes
    bubbleSort(pts, ordered_idxs);
}

// Same ar orderer but from slimmer2  ---> FAILS when trying to order 208 elements!
void orderer2 (const Puppi slimmed[NPUPPI_MAX], idx_t ordered_idxs[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    Puppi::pt_t pts[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=pts complete

    // Pre-ordering pTs and idxs
    LOOP_ORDERER_PRE: for (unsigned int i=0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        pts[i] = slimmed[i].hwPt;
        ordered_idxs[i] = i;
    }

    // Sort pts and indexes
    bubbleSort2(pts, ordered_idxs);
}

// Same as orderer2 but split into 4 arrays of 52 elements (4 x 52 = 208)
void orderer3 (const Puppi slimmed[NPUPPI_MAX],
               idx_t ordered_idxs1[NPUPPI_MAX/4], idx_t ordered_idxs2[NPUPPI_MAX/4],
               idx_t ordered_idxs3[NPUPPI_MAX/4], idx_t ordered_idxs4[NPUPPI_MAX/4])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs1 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs2 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs3 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs4 complete

    Puppi::pt_t pts[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=pts complete

    idx_t ordered_idxs[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    // Pre-ordering pTs and idxs
    LOOP_ORDERER_PRE: for (unsigned int i=0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        pts[i] = slimmed[i].hwPt;
        ordered_idxs[i] = i;
    }

    Puppi::pt_t pts1[NPUPPI_MAX/4], pts2[NPUPPI_MAX/4], pts3[NPUPPI_MAX/4], pts4[NPUPPI_MAX/4];
    #pragma HLS ARRAY_PARTITION variable=pts1 complete
    #pragma HLS ARRAY_PARTITION variable=pts2 complete
    #pragma HLS ARRAY_PARTITION variable=pts3 complete
    #pragma HLS ARRAY_PARTITION variable=pts4 complete

    // Split ordered idxs in 4 arrays
    LOOP_ORDERER_SLICE: for (unsigned int i=0; i < NPUPPI_MAX/4; i++)
    {
        #pragma HLS UNROLL

        ordered_idxs1[i] = ordered_idxs[i];
        ordered_idxs2[i] = ordered_idxs[i+NPUPPI_MAX/4];
        ordered_idxs3[i] = ordered_idxs[i+2*NPUPPI_MAX/4];
        ordered_idxs4[i] = ordered_idxs[i+3*NPUPPI_MAX/4];

        pts1[i] = pts[i];
        pts2[i] = pts[i+NPUPPI_MAX/4];
        pts3[i] = pts[i+2*NPUPPI_MAX/4];
        pts4[i] = pts[i+3*NPUPPI_MAX/4];
    }

    // Sort pts and indexes
    bubbleSort3(pts1, ordered_idxs1);
    bubbleSort3(pts2, ordered_idxs2);
    bubbleSort3(pts3, ordered_idxs3);
    bubbleSort3(pts4, ordered_idxs4);
}

// Same as orderer2 but split into 8 arrays of 26 elements (8 x 26 = 208)
void orderer4 (const Puppi slimmed[NPUPPI_MAX],
               idx_t ordered_idxs1[NPUPPI_MAX/8], idx_t ordered_idxs2[NPUPPI_MAX/8],
               idx_t ordered_idxs3[NPUPPI_MAX/8], idx_t ordered_idxs4[NPUPPI_MAX/8],
               idx_t ordered_idxs5[NPUPPI_MAX/8], idx_t ordered_idxs6[NPUPPI_MAX/8],
               idx_t ordered_idxs7[NPUPPI_MAX/8], idx_t ordered_idxs8[NPUPPI_MAX/8])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs1 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs2 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs3 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs4 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs5 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs6 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs7 complete
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs8 complete

    Puppi::pt_t pts[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=pts complete

    idx_t ordered_idxs[NPUPPI_MAX];
    #pragma HLS ARRAY_PARTITION variable=ordered_idxs complete

    // Pre-ordering pTs and idxs
    LOOP_ORDERER_PRE: for (unsigned int i=0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        pts[i] = slimmed[i].hwPt;
        ordered_idxs[i] = i;
    }

    Puppi::pt_t pts1[NPUPPI_MAX/8], pts2[NPUPPI_MAX/8], pts3[NPUPPI_MAX/8], pts4[NPUPPI_MAX/8];
    Puppi::pt_t pts5[NPUPPI_MAX/8], pts6[NPUPPI_MAX/8], pts7[NPUPPI_MAX/8], pts8[NPUPPI_MAX/8];
    #pragma HLS ARRAY_PARTITION variable=pts1 complete
    #pragma HLS ARRAY_PARTITION variable=pts2 complete
    #pragma HLS ARRAY_PARTITION variable=pts3 complete
    #pragma HLS ARRAY_PARTITION variable=pts4 complete
    #pragma HLS ARRAY_PARTITION variable=pts5 complete
    #pragma HLS ARRAY_PARTITION variable=pts6 complete
    #pragma HLS ARRAY_PARTITION variable=pts7 complete
    #pragma HLS ARRAY_PARTITION variable=pts8 complete

    // Split ordered idxs in 8 arrays
    LOOP_ORDERER_SLICE: for (unsigned int i=0; i < NPUPPI_MAX/8; i++)
    {
        #pragma HLS UNROLL

        ordered_idxs1[i] = ordered_idxs[i];
        ordered_idxs2[i] = ordered_idxs[i+NPUPPI_MAX/8];
        ordered_idxs3[i] = ordered_idxs[i+2*NPUPPI_MAX/8];
        ordered_idxs4[i] = ordered_idxs[i+3*NPUPPI_MAX/8];
        ordered_idxs5[i] = ordered_idxs[i+4*NPUPPI_MAX/8];
        ordered_idxs6[i] = ordered_idxs[i+5*NPUPPI_MAX/8];
        ordered_idxs7[i] = ordered_idxs[i+6*NPUPPI_MAX/8];
        ordered_idxs8[i] = ordered_idxs[i+7*NPUPPI_MAX/8];

        pts1[i] = pts[i];
        pts2[i] = pts[i+NPUPPI_MAX/8];
        pts3[i] = pts[i+2*NPUPPI_MAX/8];
        pts4[i] = pts[i+3*NPUPPI_MAX/8];
        pts5[i] = pts[i+4*NPUPPI_MAX/8];
        pts6[i] = pts[i+5*NPUPPI_MAX/8];
        pts7[i] = pts[i+6*NPUPPI_MAX/8];
        pts8[i] = pts[i+7*NPUPPI_MAX/8];
    }

    // Sort pts and indexes
    bubbleSort4(pts1, ordered_idxs1);
    bubbleSort4(pts2, ordered_idxs2);
    bubbleSort4(pts3, ordered_idxs3);
    bubbleSort4(pts4, ordered_idxs4);
    bubbleSort4(pts5, ordered_idxs5);
    bubbleSort4(pts6, ordered_idxs6);
    bubbleSort4(pts7, ordered_idxs7);
    bubbleSort4(pts8, ordered_idxs8);
}

// Use bitonicSort from bitonic_hybrid.h
void orderer5 (const Puppi slimmed[NPUPPI_MAX], Puppi ordered[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered complete

    // Copy-paste input to output
    LOOP_ORDERER5_COPY: for (unsigned int i=0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS UNROLL
        ordered[i] = slimmed[i];
    }

    // Do the sorting
    hybridBitonicSort::bitonicSorter<Puppi, NPUPPI_MAX, 0>::run(ordered, 0);
}

// Split in 16 ordered arrays of 13 candidates (16 x 13 = 208)
void orderer7 (const Puppi slimmed[NPUPPI_MAX],
               Puppi ordered1 [NSPLITS], Puppi ordered2 [NSPLITS],
               Puppi ordered3 [NSPLITS], Puppi ordered4 [NSPLITS],
               Puppi ordered5 [NSPLITS], Puppi ordered6 [NSPLITS],
               Puppi ordered7 [NSPLITS], Puppi ordered8 [NSPLITS],
               Puppi ordered9 [NSPLITS], Puppi ordered10[NSPLITS],
               Puppi ordered11[NSPLITS], Puppi ordered12[NSPLITS],
               Puppi ordered13[NSPLITS], Puppi ordered14[NSPLITS],
               Puppi ordered15[NSPLITS], Puppi ordered16[NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed   complete
    #pragma HLS ARRAY_PARTITION variable=ordered1  complete
    #pragma HLS ARRAY_PARTITION variable=ordered2  complete
    #pragma HLS ARRAY_PARTITION variable=ordered3  complete
    #pragma HLS ARRAY_PARTITION variable=ordered4  complete
    #pragma HLS ARRAY_PARTITION variable=ordered5  complete
    #pragma HLS ARRAY_PARTITION variable=ordered6  complete
    #pragma HLS ARRAY_PARTITION variable=ordered7  complete
    #pragma HLS ARRAY_PARTITION variable=ordered8  complete
    #pragma HLS ARRAY_PARTITION variable=ordered9  complete
    #pragma HLS ARRAY_PARTITION variable=ordered10 complete
    #pragma HLS ARRAY_PARTITION variable=ordered11 complete
    #pragma HLS ARRAY_PARTITION variable=ordered12 complete
    #pragma HLS ARRAY_PARTITION variable=ordered13 complete
    #pragma HLS ARRAY_PARTITION variable=ordered14 complete
    #pragma HLS ARRAY_PARTITION variable=ordered15 complete
    #pragma HLS ARRAY_PARTITION variable=ordered16 complete

    // Split in 8 arrays
    LOOP_ORDERER7_SPLIT: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered1 [i] = slimmed[i];
        ordered2 [i] = slimmed[i+NSPLITS];
        ordered3 [i] = slimmed[i+2*NSPLITS];
        ordered4 [i] = slimmed[i+3*NSPLITS];
        ordered5 [i] = slimmed[i+4*NSPLITS];
        ordered6 [i] = slimmed[i+5*NSPLITS];
        ordered7 [i] = slimmed[i+6*NSPLITS];
        ordered8 [i] = slimmed[i+7*NSPLITS];
        ordered9 [i] = slimmed[i+8*NSPLITS];
        ordered10[i] = slimmed[i+9*NSPLITS];
        ordered11[i] = slimmed[i+10*NSPLITS];
        ordered12[i] = slimmed[i+11*NSPLITS];
        ordered13[i] = slimmed[i+12*NSPLITS];
        ordered14[i] = slimmed[i+13*NSPLITS];
        ordered15[i] = slimmed[i+14*NSPLITS];
        ordered16[i] = slimmed[i+15*NSPLITS];
    }

    // Sort arrays
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered1  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered2  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered3  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered4  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered5  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered6  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered7  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered8  , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered9 , 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered10, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered11, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered12, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered13, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered14, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered15, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, false>::run(ordered16, 0);
}

// Same as orderer7 but split in 8 arrays of 26
void orderer7bis (const Puppi slimmed[NPUPPI_MAX],
                  Puppi ordered1 [NSPLITS], Puppi ordered2 [NSPLITS],
                  Puppi ordered3 [NSPLITS], Puppi ordered4 [NSPLITS],
                  Puppi ordered5 [NSPLITS], Puppi ordered6 [NSPLITS],
                  Puppi ordered7 [NSPLITS], Puppi ordered8 [NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed   complete
    #pragma HLS ARRAY_PARTITION variable=ordered1  complete
    #pragma HLS ARRAY_PARTITION variable=ordered2  complete
    #pragma HLS ARRAY_PARTITION variable=ordered3  complete
    #pragma HLS ARRAY_PARTITION variable=ordered4  complete
    #pragma HLS ARRAY_PARTITION variable=ordered5  complete
    #pragma HLS ARRAY_PARTITION variable=ordered6  complete
    #pragma HLS ARRAY_PARTITION variable=ordered7  complete
    #pragma HLS ARRAY_PARTITION variable=ordered8  complete

    // Split in 8 arrays
    LOOP_ORDERER7_SPLIT: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered1[i] = slimmed[i];
        ordered2[i] = slimmed[i+NSPLITS];
        ordered3[i] = slimmed[i+2*NSPLITS];
        ordered4[i] = slimmed[i+3*NSPLITS];
        ordered5[i] = slimmed[i+4*NSPLITS];
        ordered6[i] = slimmed[i+5*NSPLITS];
        ordered7[i] = slimmed[i+6*NSPLITS];
        ordered8[i] = slimmed[i+7*NSPLITS];
    }

    // Sort arrays
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered1, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered2, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered3, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered4, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered5, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered6, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered7, 0);
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered8, 0);
}

// order a single sub-array
void single_orderer (Puppi ordered[NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=ordered complete
    hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered, 0);
}

// Same as orderer7bis but use single_orderer
void orderer7c (const Puppi slimmed[NPUPPI_MAX],
                Puppi ordered1 [NSPLITS], Puppi ordered2 [NSPLITS],
                Puppi ordered3 [NSPLITS], Puppi ordered4 [NSPLITS],
                Puppi ordered5 [NSPLITS], Puppi ordered6 [NSPLITS],
                Puppi ordered7 [NSPLITS], Puppi ordered8 [NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed   complete
    #pragma HLS ARRAY_PARTITION variable=ordered1  complete
    #pragma HLS ARRAY_PARTITION variable=ordered2  complete
    #pragma HLS ARRAY_PARTITION variable=ordered3  complete
    #pragma HLS ARRAY_PARTITION variable=ordered4  complete
    #pragma HLS ARRAY_PARTITION variable=ordered5  complete
    #pragma HLS ARRAY_PARTITION variable=ordered6  complete
    #pragma HLS ARRAY_PARTITION variable=ordered7  complete
    #pragma HLS ARRAY_PARTITION variable=ordered8  complete

    // Split in 8 arrays
    LOOP_ORDERER7_SPLIT: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered1[i] = slimmed[i];
        ordered2[i] = slimmed[i+NSPLITS];
        ordered3[i] = slimmed[i+2*NSPLITS];
        ordered4[i] = slimmed[i+3*NSPLITS];
        ordered5[i] = slimmed[i+4*NSPLITS];
        ordered6[i] = slimmed[i+5*NSPLITS];
        ordered7[i] = slimmed[i+6*NSPLITS];
        ordered8[i] = slimmed[i+7*NSPLITS];
    }

    // Sort arrays
    single_orderer(ordered1);
    single_orderer(ordered2);
    single_orderer(ordered3);
    single_orderer(ordered4);
    single_orderer(ordered5);
    single_orderer(ordered6);
    single_orderer(ordered7);
    single_orderer(ordered8);
}

// Order all 208 elements at once
void orderer7d (const Puppi slimmed[NPUPPI_MAX], Puppi ordered[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed  complete
    #pragma HLS ARRAY_PARTITION variable=ordered  complete

    LOOP_ORDERER7D: for (unsigned int i=0; i < NPUPPI_MAX; i++)
    {
        #pragma HLS unroll
        ordered[i] = slimmed[i];
    }
    hybridBitonicSort::bitonicSorter<Puppi, NPUPPI_MAX, 0, true>::run(ordered, 0);
}

void orderer7e (const Puppi slimmed[NPUPPI_MAX], Puppi ordered[NSUBARR][NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered complete dim=2

    // Split in 8 arrays
    LOOP_ORDERER7E_FILL: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered[0][i] = slimmed[i];
        ordered[1][i] = slimmed[i+NSPLITS];
        ordered[2][i] = slimmed[i+2*NSPLITS];
        ordered[3][i] = slimmed[i+3*NSPLITS];
        ordered[4][i] = slimmed[i+4*NSPLITS];
        ordered[5][i] = slimmed[i+5*NSPLITS];
        ordered[6][i] = slimmed[i+6*NSPLITS];
        ordered[7][i] = slimmed[i+7*NSPLITS];
    }

    LOOP_ORDERER7E_SORT: for (unsigned int i=0; i < NSUBARR; i++)
    {
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i], 0);
    }
}

void orderer7f (const Puppi slimmed[NPUPPI_MAX], Puppi ordered[NSUBARR][NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered complete dim=2

    // Split in 8 arrays
    LOOP_ORDERER7E_FILL: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered[0][i] = slimmed[i];
        ordered[1][i] = slimmed[i+NSPLITS];
        ordered[2][i] = slimmed[i+2*NSPLITS];
        ordered[3][i] = slimmed[i+3*NSPLITS];
        ordered[4][i] = slimmed[i+4*NSPLITS];
        ordered[5][i] = slimmed[i+5*NSPLITS];
        ordered[6][i] = slimmed[i+6*NSPLITS];
        ordered[7][i] = slimmed[i+7*NSPLITS];
    }

    // Sort the 8 arrays
    LOOP_ORDERER7E_SORT: for (unsigned int i=0; i < NSUBARR / 2; i++)
    {
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i]  , 0);
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i+1], 0);
    }
}

void orderer7g (const Puppi slimmed[NPUPPI_MAX], Puppi ordered[NSUBARR][NSPLITS])
// II Violation! due to limited memory ports (II = 1). Please consider using a memory core with more ports or partitioning the array
{
    #pragma HLS ARRAY_PARTITION variable=slimmed complete
    #pragma HLS ARRAY_PARTITION variable=ordered complete dim=0

    // Split in 8 arrays
    LOOP_ORDERER7E_FILL: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered[0][i] = slimmed[i];
        ordered[1][i] = slimmed[i+NSPLITS];
        ordered[2][i] = slimmed[i+2*NSPLITS];
        ordered[3][i] = slimmed[i+3*NSPLITS];
        ordered[4][i] = slimmed[i+4*NSPLITS];
        ordered[5][i] = slimmed[i+5*NSPLITS];
        ordered[6][i] = slimmed[i+6*NSPLITS];
        ordered[7][i] = slimmed[i+7*NSPLITS];
    }

    LOOP_ORDERER7E_SORT: for (unsigned int i = 0; i < NSUBARR/4; i++)
    {
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i]  , 0);
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i+1], 0);
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i+2], 0);
        hybridBitonicSort::bitonicSorter<Puppi, NSPLITS, 0, true>::run(ordered[i+3], 0);
    }
}

// Same as orderer7 but using bubbleSort8
void orderer8 (const Puppi slimmed[NPUPPI_MAX],
               Puppi ordered1 [NSPLITS], Puppi ordered2 [NSPLITS],
               Puppi ordered3 [NSPLITS], Puppi ordered4 [NSPLITS],
               Puppi ordered5 [NSPLITS], Puppi ordered6 [NSPLITS],
               Puppi ordered7 [NSPLITS], Puppi ordered8 [NSPLITS],
               Puppi ordered9 [NSPLITS], Puppi ordered10[NSPLITS],
               Puppi ordered11[NSPLITS], Puppi ordered12[NSPLITS],
               Puppi ordered13[NSPLITS], Puppi ordered14[NSPLITS],
               Puppi ordered15[NSPLITS], Puppi ordered16[NSPLITS])
{
    #pragma HLS ARRAY_PARTITION variable=slimmed   complete
    #pragma HLS ARRAY_PARTITION variable=ordered1  complete
    #pragma HLS ARRAY_PARTITION variable=ordered2  complete
    #pragma HLS ARRAY_PARTITION variable=ordered3  complete
    #pragma HLS ARRAY_PARTITION variable=ordered4  complete
    #pragma HLS ARRAY_PARTITION variable=ordered5  complete
    #pragma HLS ARRAY_PARTITION variable=ordered6  complete
    #pragma HLS ARRAY_PARTITION variable=ordered7  complete
    #pragma HLS ARRAY_PARTITION variable=ordered8  complete
    #pragma HLS ARRAY_PARTITION variable=ordered9  complete
    #pragma HLS ARRAY_PARTITION variable=ordered10 complete
    #pragma HLS ARRAY_PARTITION variable=ordered11 complete
    #pragma HLS ARRAY_PARTITION variable=ordered12 complete
    #pragma HLS ARRAY_PARTITION variable=ordered13 complete
    #pragma HLS ARRAY_PARTITION variable=ordered14 complete
    #pragma HLS ARRAY_PARTITION variable=ordered15 complete
    #pragma HLS ARRAY_PARTITION variable=ordered16 complete

    // Split in 8 arrays
    LOOP_ORDERER8_SPLIT: for (unsigned int i=0; i < NSPLITS; i++)
    {
        #pragma HLS UNROLL
        ordered1 [i] = slimmed[i];
        ordered2 [i] = slimmed[i+NSPLITS];
        ordered3 [i] = slimmed[i+2*NSPLITS];
        ordered4 [i] = slimmed[i+3*NSPLITS];
        ordered5 [i] = slimmed[i+4*NSPLITS];
        ordered6 [i] = slimmed[i+5*NSPLITS];
        ordered7 [i] = slimmed[i+6*NSPLITS];
        ordered8 [i] = slimmed[i+7*NSPLITS];
        ordered9 [i] = slimmed[i+8*NSPLITS];
        ordered10[i] = slimmed[i+9*NSPLITS];
        ordered11[i] = slimmed[i+10*NSPLITS];
        ordered12[i] = slimmed[i+11*NSPLITS];
        ordered13[i] = slimmed[i+12*NSPLITS];
        ordered14[i] = slimmed[i+13*NSPLITS];
        ordered15[i] = slimmed[i+14*NSPLITS];
        ordered16[i] = slimmed[i+15*NSPLITS];
    }

    // Sort arrays
    bubbleSort8(ordered1 );
    bubbleSort8(ordered2 );
    bubbleSort8(ordered3 );
    bubbleSort8(ordered4 );
    bubbleSort8(ordered5 );
    bubbleSort8(ordered6 );
    bubbleSort8(ordered7 );
    bubbleSort8(ordered8 );
    bubbleSort8(ordered9 );
    bubbleSort8(ordered10);
    bubbleSort8(ordered11);
    bubbleSort8(ordered12);
    bubbleSort8(ordered13);
    bubbleSort8(ordered14);
    bubbleSort8(ordered15);
    bubbleSort8(ordered16);
}


// ------------------------------------------------------------------
// Utils for merge-sorting

// get_bitonic_sequence
void get_bitonic_sequenceA(const Puppi in1[NSPLITS], const Puppi in2[NSPLITS], Puppi bitonic[2*NSPLITS]) {
    //#pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=bitonic complete
    make_bitonic_loop: for(int id = 0, ia=NSPLITS-1; id<NSPLITS; id++, ia--)
    {
        #pragma HLS UNROLL
        bitonic[id] = in1[ia];
        bitonic[id+(NSPLITS)] = in2[id];
    }
}

void get_bitonic_sequenceB(const Puppi in1[2*NSPLITS], const Puppi in2[2*NSPLITS], Puppi bitonic[4*NSPLITS]) {
    //#pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=bitonic complete
    make_bitonic_loop: for(int id = 0, ia=2*NSPLITS-1; id<2*NSPLITS; id++, ia--)
    {
        #pragma HLS UNROLL
        bitonic[id] = in1[ia];
        bitonic[id+(2*NSPLITS)] = in2[id];
    }
}

void get_bitonic_sequenceC(const Puppi in1[4*NSPLITS], const Puppi in2[4*NSPLITS], Puppi bitonic[8*NSPLITS]) {
    //#pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=bitonic complete
    make_bitonic_loop: for(int id = 0, ia=4*NSPLITS-1; id<4*NSPLITS; id++, ia--)
    {
        #pragma HLS UNROLL
        bitonic[id] = in1[ia];
        bitonic[id+(4*NSPLITS)] = in2[id];
    }
}

void get_bitonic_sequenceD(const Puppi in1[8*NSPLITS], const Puppi in2[8*NSPLITS], Puppi bitonic[16*NSPLITS]) {
    #pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=bitonic complete
    make_bitonic_loop: for(int id = 0, ia=8*NSPLITS-1; id<8*NSPLITS; id++, ia--)
    {
        #pragma HLS UNROLL
        bitonic[id] = in1[ia];
        bitonic[id+(8*NSPLITS)] = in2[id];
    }
}

// merge_sort
void merge_sortA(const Puppi in1[NSPLITS], const Puppi in2[NSPLITS], Puppi sorted_out[2*NSPLITS]) {
    //#pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=sorted_out complete
    get_bitonic_sequenceA(in1, in2, sorted_out);
    hybridBitonicSort::bitonicMerger<Puppi, 2*NSPLITS, 0>::run(sorted_out, 0);
}

void merge_sortB(const Puppi in1[2*NSPLITS], const Puppi in2[2*NSPLITS], Puppi sorted_out[4*NSPLITS]) {
    //#pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=sorted_out complete
    get_bitonic_sequenceB(in1, in2, sorted_out);
    hybridBitonicSort::bitonicMerger<Puppi, 4*NSPLITS, 0>::run(sorted_out, 0);
}

void merge_sortC(const Puppi in1[4*NSPLITS], const Puppi in2[4*NSPLITS], Puppi sorted_out[8*NSPLITS]) {
    //#pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=sorted_out complete
    get_bitonic_sequenceC(in1, in2, sorted_out);
    hybridBitonicSort::bitonicMerger<Puppi, 8*NSPLITS, 0>::run(sorted_out, 0);
}

void merge_sortD(const Puppi in1[8*NSPLITS], const Puppi in2[8*NSPLITS], Puppi sorted_out[16*NSPLITS]) {
    #pragma HLS inline
    #pragma HLS array_partition variable=in1 complete
    #pragma HLS array_partition variable=in2 complete
    #pragma HLS array_partition variable=sorted_out complete
    get_bitonic_sequenceD(in1, in2, sorted_out);
    hybridBitonicSort::bitonicMerger<Puppi, 16*NSPLITS, 0>::run(sorted_out, 0);
}

// ------------------------------------------------------------------
// Merger: merge ordered arrays with bitonicMerger
void merger (Puppi ordered1 [NSPLITS], Puppi ordered2 [NSPLITS],
             Puppi ordered3 [NSPLITS], Puppi ordered4 [NSPLITS],
             Puppi ordered5 [NSPLITS], Puppi ordered6 [NSPLITS],
             Puppi ordered7 [NSPLITS], Puppi ordered8 [NSPLITS],
             Puppi ordered9 [NSPLITS], Puppi ordered10[NSPLITS],
             Puppi ordered11[NSPLITS], Puppi ordered12[NSPLITS],
             Puppi ordered13[NSPLITS], Puppi ordered14[NSPLITS],
             Puppi ordered15[NSPLITS], Puppi ordered16[NSPLITS],
             Puppi merged[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=ordered1  complete
    #pragma HLS ARRAY_PARTITION variable=ordered2  complete
    #pragma HLS ARRAY_PARTITION variable=ordered3  complete
    #pragma HLS ARRAY_PARTITION variable=ordered4  complete
    #pragma HLS ARRAY_PARTITION variable=ordered5  complete
    #pragma HLS ARRAY_PARTITION variable=ordered6  complete
    #pragma HLS ARRAY_PARTITION variable=ordered7  complete
    #pragma HLS ARRAY_PARTITION variable=ordered8  complete
    #pragma HLS ARRAY_PARTITION variable=ordered9  complete
    #pragma HLS ARRAY_PARTITION variable=ordered10 complete
    #pragma HLS ARRAY_PARTITION variable=ordered11 complete
    #pragma HLS ARRAY_PARTITION variable=ordered12 complete
    #pragma HLS ARRAY_PARTITION variable=ordered13 complete
    #pragma HLS ARRAY_PARTITION variable=ordered14 complete
    #pragma HLS ARRAY_PARTITION variable=ordered15 complete
    #pragma HLS ARRAY_PARTITION variable=ordered16 complete
    #pragma HLS ARRAY_PARTITION variable=merged    complete

    // Recursively merge and sort in pairs
    // Step 1
    Puppi merge1[2*NSPLITS];
    merge_sortA(ordered1, ordered2, merge1);

    Puppi merge2[2*NSPLITS];
    merge_sortA(ordered3, ordered4, merge2);

    Puppi merge3[2*NSPLITS];
    merge_sortA(ordered5, ordered6, merge3);

    Puppi merge4[2*NSPLITS];
    merge_sortA(ordered7, ordered8, merge4);

    Puppi merge5[2*NSPLITS];
    merge_sortA(ordered9, ordered10, merge5);

    Puppi merge6[2*NSPLITS];
    merge_sortA(ordered11, ordered12, merge6);

    Puppi merge7[2*NSPLITS];
    merge_sortA(ordered13, ordered14, merge7);

    Puppi merge8[2*NSPLITS];
    merge_sortA(ordered15, ordered16, merge8);

    // Step 2
    Puppi merge9[4*NSPLITS];
    merge_sortB(merge1, merge2, merge9);

    Puppi merge10[4*NSPLITS];
    merge_sortB(merge3, merge4, merge10);

    Puppi merge11[4*NSPLITS];
    merge_sortB(merge5, merge6, merge11);

    Puppi merge12[4*NSPLITS];
    merge_sortB(merge7, merge8, merge12);

    // Step 3
    Puppi merge13[8*NSPLITS];
    merge_sortC(merge9, merge10, merge13);

    Puppi merge14[8*NSPLITS];
    merge_sortC(merge11, merge12, merge14);

    // Final
    merge_sortD(merge13, merge14, merged);
}

// Same as merger but using 8 arrays of 26 elements
void merger7bis (Puppi ordered1 [NSPLITS], Puppi ordered2 [NSPLITS],
                 Puppi ordered3 [NSPLITS], Puppi ordered4 [NSPLITS],
                 Puppi ordered5 [NSPLITS], Puppi ordered6 [NSPLITS],
                 Puppi ordered7 [NSPLITS], Puppi ordered8 [NSPLITS],
                 Puppi merged[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=ordered1  complete
    #pragma HLS ARRAY_PARTITION variable=ordered2  complete
    #pragma HLS ARRAY_PARTITION variable=ordered3  complete
    #pragma HLS ARRAY_PARTITION variable=ordered4  complete
    #pragma HLS ARRAY_PARTITION variable=ordered5  complete
    #pragma HLS ARRAY_PARTITION variable=ordered6  complete
    #pragma HLS ARRAY_PARTITION variable=ordered7  complete
    #pragma HLS ARRAY_PARTITION variable=ordered8  complete
    #pragma HLS ARRAY_PARTITION variable=merged    complete

    // Recursively merge and sort in pairs
    // Step 1
    Puppi merge1[2*NSPLITS];
    merge_sortA(ordered1, ordered2, merge1);

    Puppi merge2[2*NSPLITS];
    merge_sortA(ordered3, ordered4, merge2);

    Puppi merge3[2*NSPLITS];
    merge_sortA(ordered5, ordered6, merge3);

    Puppi merge4[2*NSPLITS];
    merge_sortA(ordered7, ordered8, merge4);

    // Step 2
    Puppi merge5[4*NSPLITS];
    merge_sortB(merge1, merge2, merge5);

    Puppi merge6[4*NSPLITS];
    merge_sortB(merge3, merge4, merge6);

    // Final
    merge_sortC(merge5, merge6, merged);
}

// Same as merger7bis but starting from orderer7f
void merger7f (Puppi ordered[NSUBARR][NSPLITS], Puppi merged[NPUPPI_MAX])
{
    #pragma HLS ARRAY_PARTITION variable=ordered complete dim=2
    #pragma HLS ARRAY_PARTITION variable=merged  complete

    // original                                                       // --> v0
    //#pragma HLS ALLOCATION instances=merge_sortA limit=1 function   // --> v1 |
    //#pragma HLS ALLOCATION instances=merge_sortA limit=2 function   // --> v2 |
    //#pragma HLS ALLOCATION instances=merge_sortB limit=1 function   // --> v3 |-> all the same v1-4
    //#pragma HLS ALLOCATION instances=merge_sortA limit=1 function   // --> v4 |
    //#pragma HLS ALLOCATION instances=merge_sortB limit=1 function   //     v4 |
    // remove inlinings from merge_sortA                              // --> v5
    //#pragma HLS ALLOCATION instances=merge_sortA limit=1 function   // + remove inlinings from merge_sortA --> v6 // same as v5
    // remove inlinings from merge_sortB                              // --> v7
    // remove inlinings from merge_sortA and merge_sortB              // --> v8
    // remove inlinings from merge_sortC                              // --> v9  // almost identical to v1-4
    // remove inlinings from merge_sortA, merge_sortB and merge_sortC // --> v10 // almost identical to v8
    // remove inlinings from all merge_sort and all bitonic_sequence  // --> v11 // best so far
    // next:
    //   - test not inlining + ALLOCATION combination

    // Recursively merge and sort in pairs
    // Step 1
    Puppi merge1[2*NSPLITS];
    merge_sortA(ordered[0], ordered[1], merge1);

    Puppi merge2[2*NSPLITS];
    merge_sortA(ordered[2], ordered[3], merge2);

    Puppi merge3[2*NSPLITS];
    merge_sortA(ordered[4], ordered[5], merge3);

    Puppi merge4[2*NSPLITS];
    merge_sortA(ordered[6], ordered[7], merge4);

    // Step 2
    Puppi merge5[4*NSPLITS];
    merge_sortB(merge1, merge2, merge5);

    Puppi merge6[4*NSPLITS];
    merge_sortB(merge3, merge4, merge6);

    // Final
    merge_sortC(merge5, merge6, merged);
}


// ------------------------------------------------------------------
// Get maximum deltaVz
Puppi::z0_t get_max_dVz (Puppi::z0_t z0, Puppi::z0_t z1, Puppi::z0_t z2)
{
    #pragma HLS inline

    // Get all dVz values
    Puppi::z0_t dVz_01 = z0 - z1;
    Puppi::z0_t dVz_02 = z0 - z2;
    Puppi::z0_t dVz_12 = z1 - z2;

    // std library max
    Puppi::z0_t max = std::max(dVz_01, std::max(dVz_02, dVz_12));

    return max;
}

// ------------------------------------------------------------------
// DeltaR methods
inline dr2_t drToHwDr2 (float dr)
{
    return dr2_t(round(std::pow(dr/Puppi::ETAPHI_LSB,2)));
}

inline float hwDr2ToDr (dr2_t dr2)
{
    return std::sqrt(dr2.to_int()*Puppi::ETAPHI_LSB*Puppi::ETAPHI_LSB);
}

ap_int<Puppi::eta_t::width+1> deltaEta (Puppi::eta_t eta1, Puppi::eta_t eta2)
{
    #pragma HLS latency min=1
    #pragma HLS inline off
    return eta1 - eta2;
}

inline dr2_t deltaR2 (const Puppi & p1, const Puppi & p2)
{
    auto dphi = p1.hwPhi - p2.hwPhi;
    if (dphi > Puppi::INT_PI) dphi -= Puppi::INT_2PI;
    else if (dphi < -Puppi::INT_PI) dphi += Puppi::INT_2PI;
    auto deta = p1.hwEta - p2.hwEta;
    return dphi*dphi + deta*deta;
}

inline dr2_t deltaR2_slow (const Puppi & p1, const Puppi & p2)
{
    auto dphi = p1.hwPhi - p2.hwPhi;
    if (dphi > Puppi::INT_PI) dphi -= Puppi::INT_2PI;
    else if (dphi < -Puppi::INT_PI) dphi += Puppi::INT_2PI;
    auto deta = deltaEta(p1.hwEta, p2.hwEta);
    return dphi*dphi + deta*deta;
}

// Get min deltaR slow
dr2_t get_min_deltaR2_slow (const Puppi p0, const Puppi p1, const Puppi p2)
{
    // Get the 3 dR values
    dr2_t dr2_01 = deltaR2_slow(p0, p1);
    dr2_t dr2_02 = deltaR2_slow(p0, p2);
    dr2_t dr2_12 = deltaR2_slow(p1, p2);

    // std library min
    dr2_t min = std::min(dr2_01, std::min(dr2_02, dr2_12));

    return min;
}

// Get min deltaR
dr2_t get_min_deltaR2 (const Puppi p0, const Puppi p1, const Puppi p2)
{
    // Get the 3 dR values
    dr2_t dr2_01 = deltaR2(p0, p1);
    dr2_t dr2_02 = deltaR2(p0, p2);
    dr2_t dr2_12 = deltaR2(p1, p2);

    // std library min
    dr2_t min = std::min(dr2_01, std::min(dr2_02, dr2_12));

    return min;
}

// ------------------------------------------------------------------
// Prepare triplet inputs features:
//  0. pi2_pt
//  1. pi1_pt
//  2. m_01
//  3. triplet_charge
//  4. dVz_02
//  5. pi0_pt
//  6. triplet_pt
//  7. m_02
//  8. triplet_maxdVz
//  9. triplet_mindR
// 10. pi2_eta
void get_triplet_inputs (const Puppi selected[NPUPPI_SEL], idx_t idx0, idx_t idx1, idx_t idx2, w3p_bdt::input_t BDT_inputs[w3p_bdt::n_features])
{
    // Fill BDT input for triplet
    BDT_inputs[0]  = selected[idx2].hwPt;
    BDT_inputs[1]  = selected[idx1].hwPt;
    BDT_inputs[2]  = 0.2;
    BDT_inputs[3]  = selected[idx0].charge() + selected[idx1].charge() + selected[idx2].charge();
    BDT_inputs[4]  = selected[idx0].hwZ0 - selected[idx2].hwZ0;
    BDT_inputs[5]  = selected[idx0].hwPt;
    BDT_inputs[6]  = selected[idx0].hwPt + selected[idx1].hwPt + selected[idx2].hwPt;
    BDT_inputs[7]  = 0.7; //  7. m_02
    BDT_inputs[8]  = get_max_dVz(selected[idx0].hwZ0, selected[idx1].hwZ0, selected[idx2].hwZ0);
    BDT_inputs[9]  = get_min_deltaR2_slow(selected[idx0], selected[idx1], selected[idx2]);
    BDT_inputs[10] = selected[idx2].hwEta;
}

// ------------------------------------------------------------------
// Get all event inputs
// We use 10 candidates and 2 pivots --> 8 triplets:
// (0,1,2)-(0,1,3)-(0,1,4)-(0,1,5)-(0,1,6)-(0,1,7)-(0,1,8)-(0,1,9)
void get_event_inputs (const Puppi selected[NPUPPI_SEL], w3p_bdt::input_t BDT_inputs[NTRIPLETS][w3p_bdt::n_features])
{
    #pragma HLS ARRAY_PARTITION variable=selected complete
    #pragma HLS ARRAY_PARTITION variable=BDT_inputs complete dim=0

    LOOP_EVENT_INPUTS: for (unsigned int i = 0; i < NTRIPLETS; i++)
    {
        #pragma HLS unroll
        idx_t third_idx = i + 2; // 2 pivots
        get_triplet_inputs(selected, 0, 1, third_idx, BDT_inputs[i]);
    }
}

// ------------------------------------------------------------------
// Get BDT scores of the selected triplets
void get_event_scores (w3p_bdt::input_t BDT_inputs[NTRIPLETS][w3p_bdt::n_features], w3p_bdt::score_t BDT_scores[NTRIPLETS])
{
    #pragma HLS ARRAY_PARTITION variable=BDT_inputs complete dim=0
    #pragma HLS ARRAY_PARTITION variable=BDT_scores complete

    // Get the BDT scores
    LOOP_EVENT_SCORES: for (unsigned int i = 0; i < NTRIPLETS; i++)
    {
        #pragma HLS unroll
        w3p_bdt::bdt.decision_function(BDT_inputs[i], &BDT_scores[i]);
    }
}

// ------------------------------------------------------------------
// Get highest BDT score
void get_highest_score (w3p_bdt::score_t BDT_scores[NTRIPLETS], w3p_bdt::score_t & high_score)
{
    #pragma HLS ARRAY_PARTITION variable=BDT_scores complete

    // Sort BDT scores in descending order
    hybridBitonicSort::bitonicSorter<w3p_bdt::score_t, NTRIPLETS, 0, true>::run(BDT_scores, 0);

    // Get the highest score
    high_score = BDT_scores[0];
}

// ------------------------------------------------------------------
// Full EventProcessor
void EventProcessor (const Puppi input[NPUPPI_MAX], Puppi selected[NPUPPI_SEL])
{
    #pragma HLS ARRAY_PARTITION variable=input complete
    #pragma HLS ARRAY_PARTITION variable=selected complete

    // Mask candidates
    ap_uint<NPUPPI_MAX> masked;
    masker(input, masked);

    // Replace masked candidates with dummy
    Puppi slimmed[NPUPPI_MAX];
    slimmer2(input, masked, slimmed);

    // Split in arrays and sort them according to pT
    Puppi ordered1[NSPLITS] , ordered2[NSPLITS] , ordered3[NSPLITS] , ordered4[NSPLITS] ,
          ordered5[NSPLITS] , ordered6[NSPLITS] , ordered7[NSPLITS] , ordered8[NSPLITS] ,
          ordered9[NSPLITS] , ordered10[NSPLITS], ordered11[NSPLITS], ordered12[NSPLITS],
          ordered13[NSPLITS], ordered14[NSPLITS], ordered15[NSPLITS], ordered16[NSPLITS];
    orderer7(slimmed, 
             ordered1 , ordered2 , ordered3 , ordered4 ,
             ordered5 , ordered6 , ordered7 , ordered8 ,
             ordered9 , ordered10, ordered11, ordered12,
             ordered13, ordered14, ordered15, ordered16);
    
    // Merge the sorted split-arrays
    Puppi merged[NPUPPI_MAX];
    merger(ordered1, ordered2, ordered3, ordered4,
           ordered5, ordered6, ordered7, ordered8,
           ordered9, ordered10, ordered11, ordered12,
           ordered13, ordered14, ordered15, ordered16,
           merged);

    // Select only highest pT ordered-candidates
    for (unsigned int i = 0; i < NPUPPI_SEL; i++)
    {
        #pragma HLS UNROLL
        selected[i] = merged[i];
    }
}

// EventProcessor7bis - same as event processor, but with 8 arrays
void EventProcessor7bis (const Puppi input[NPUPPI_MAX], w3p_bdt::score_t & max_score)
{
    #pragma HLS ARRAY_PARTITION variable=input complete

    // Mask candidates
    ap_uint<NPUPPI_MAX> masked;
    masker(input, masked);

    // Replace masked candidates with dummy
    Puppi slimmed[NPUPPI_MAX];
    slimmer2(input, masked, slimmed);

    // Split in arrays and sort them according to pT
    Puppi ordered1[NSPLITS], ordered2[NSPLITS], ordered3[NSPLITS], ordered4[NSPLITS],
          ordered5[NSPLITS], ordered6[NSPLITS], ordered7[NSPLITS], ordered8[NSPLITS];
    orderer7bis(slimmed,
                ordered1, ordered2, ordered3, ordered4,
                ordered5, ordered6, ordered7, ordered8);
    
    // Merge the sorted split-arrays
    Puppi merged[NPUPPI_MAX];
    merger7bis(ordered1, ordered2, ordered3, ordered4,
               ordered5, ordered6, ordered7, ordered8,
               merged);

    // Select only highest pT ordered-candidates
    Puppi selected[NPUPPI_SEL];
    #pragma HLS ARRAY_PARTITION variable=selected complete
    LOOP_EP_SELECTED: for (unsigned int i = 0; i < NPUPPI_SEL; i++)
    {
        #pragma HLS UNROLL
        selected[i] = merged[i];
    }

    // Get inputs for each triplet
    w3p_bdt::input_t BDT_inputs[NTRIPLETS][w3p_bdt::n_features];
    get_event_inputs(selected, BDT_inputs);

    // Get BDT score for each triplet
    w3p_bdt::score_t BDT_scores[NTRIPLETS];
    get_event_scores(BDT_inputs, BDT_scores);

    // Get highest BDT score among triplets
    get_highest_score(BDT_scores, max_score);
}

// EventProcessor7f - same as EventProcessor7bis, but using orderer7f and merger7f
void EventProcessor7f (const Puppi input[NPUPPI_MAX], w3p_bdt::score_t & max_score)
{
    #pragma HLS ARRAY_PARTITION variable=input complete

    // Mask candidates
    ap_uint<NPUPPI_MAX> masked;
    masker(input, masked);

    // Replace masked candidates with dummy
    Puppi slimmed[NPUPPI_MAX];
    slimmer2(input, masked, slimmed);

    // Split in arrays and sort them according to pT
    // Make 2D array of [NSUBARR]x[NSPLITS] = [8]x[26] = 208 for ordering and merging
    Puppi ordered[NSUBARR][NSPLITS];
    orderer7f(slimmed, ordered);
    
    // Merge the sorted split-arrays
    Puppi merged[NPUPPI_MAX];
    merger7f(ordered, merged);

    // Select only highest pT ordered-candidates
    Puppi selected[NPUPPI_SEL];
    #pragma HLS ARRAY_PARTITION variable=selected complete
    LOOP_EP_SELECTED: for (unsigned int i = 0; i < NPUPPI_SEL; i++)
    {
        #pragma HLS UNROLL
        selected[i] = merged[i];
    }

    // Get inputs for each triplet
    w3p_bdt::input_t BDT_inputs[NTRIPLETS][w3p_bdt::n_features];
    get_event_inputs(selected, BDT_inputs);

    // Get BDT score for each triplet
    w3p_bdt::score_t BDT_scores[NTRIPLETS];
    get_event_scores(BDT_inputs, BDT_scores);

    // Get highest BDT score among triplets
    get_highest_score(BDT_scores, max_score);
}
