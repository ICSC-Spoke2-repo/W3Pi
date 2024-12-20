#ifndef W3P_EMULATOR_H
#define W3P_EMULATOR_H

#include "data.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <bitset>
#include <vector>
#include <algorithm>

// ---------------------
// ----- REFERENCE -----
// ---------------------
void w3p_emulator(const std::vector<uint64_t> input_stream[NLINKS], std::vector<Puppi> output_stream[NLINKS]);

#endif
