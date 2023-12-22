#ifndef ALGO_H
#define ALGO_H

#include "data.h"

#define DEBUG false

#define NPUPPI_MAX 216
#define NISO_MAX 12
#define NTRIPLETS_MAX 30

typedef ap_uint<24> dr2_t;

inline dr2_t drToHwDr2(float dr) { return dr2_t(round(std::pow(dr/Puppi::ETAPHI_LSB,2))); }

// w3p HLS implementation
void event_preparator (const Puppi input[NPUPPI_MAX], Puppi & pivot, Triplet triplets[NTRIPLETS_MAX], bool masked_triplets[NTRIPLETS_MAX]);
void event_preparator_ref (unsigned int npuppi, const Puppi input[NPUPPI_MAX], Puppi & pivot, Triplet triplets[NTRIPLETS_MAX], bool masked_triplets[NTRIPLETS_MAX]);

#endif
