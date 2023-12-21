#ifndef ALGO_H
#define ALGO_H

#include "data.h"

#define DEBUG false

#define NPUPPI_MAX 216
#define NISO_MAX 12
#define NTRIPLETS_MAX 20

typedef ap_uint<24> dr2_t;

inline dr2_t drToHwDr2(float dr) { return dr2_t(round(std::pow(dr/Puppi::ETAPHI_LSB,2))); }

void compute_isolated_w3p(const Puppi in[NPUPPI_MAX], Puppi out[NISO_MAX], Puppi::pt_t out_absiso[NISO_MAX]);

void compute_isolated_ref(unsigned int npuppi, const Puppi in[NPUPPI_MAX], Puppi out[NISO_MAX], Puppi::pt_t out_absiso[NISO_MAX], bool verbose=false);


// w3p HLS implementation
void event_preparator (const Puppi input[NPUPPI_MAX], Puppi & pivot);
void event_preparator_ref (unsigned int npuppi, const Puppi input[NPUPPI_MAX], Puppi & pivot);

#endif
