#pragma once

#include <stdint.h>

// A pin's GPIO input register and bit
struct Pad {
  volatile uint32_t *pdir;
  uint32_t mask;
};

struct Stats {
  int pulses;
  long wmin, wmax;  // pulse width [us]
  long pmin, pmax;  // rise to rise [us]
};
