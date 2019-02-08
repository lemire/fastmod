#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "fastmod.h"


bool testdivsigned(int32_t min, int32_t max, bool verbose) {
  
  int32_t d = 95;
  uint64_t M = computeM_s32(d);
  
  int32_t computedFastDiv = fastdiv_s32(min, M, d);
  if (max != computedFastDiv) {
    printf("problem  nah");  
    return false;
  }
  return true;
}
