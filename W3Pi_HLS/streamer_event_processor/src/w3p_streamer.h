#ifndef W3P_STREAMER_H
#define W3P_STREAMER_H

#include "data.h"

// --------------------
// ----- FIRMWARE -----
// --------------------
void w3p_streamer( hls::stream<uint64_t> input[NLINKS], hls::stream<Puppi> output[NLINKS]);

#endif
