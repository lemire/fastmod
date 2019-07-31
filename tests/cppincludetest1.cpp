#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "fastmod.h"

using namespace fastmod;


bool testunsigned64(uint64_t min, uint64_t max, bool verbose) {
  for (uint64_t d = min; (d <= max) && (d >= min); d++) {
    if (d == 0) {
      printf("skipping d = 0\n");
      continue;
    }
    __uint128_t M64 = computeM_u64(d);
    if (verbose)
      printf("d = %" PRIu64 " (unsigned 64-bit) ", d);
    else
      printf(".");
    fflush(NULL);
    for(uint64_t a64 = UINT64_C(0x10000000000000) /* 1 << 52 */; a64 < UINT64_C(0x10000000000000) + UINT64_C(0x10000); ++a64) {
      uint64_t computedFastMod = fastmod_u64(a64, M64, d);
      uint64_t computedMod = a64 % d;
      if (computedMod != computedFastMod) {
        printf(
            "(bad u64 fastmod) problem with divisor %" PRIu64 " and dividend %" PRIu64 " \n",
            d, a64);
        printf("expected %" PRIu64 " mod %" PRIu64 " = %" PRIu64 " \n", a64, d, computedMod);
        printf("got %" PRIu64 " mod %" PRIu64 " = %" PRIu64 " \n", a64, d, computedFastMod);
        return false;
      }
    }
    if (verbose)
      printf("ok!\n");
  }
  if (verbose)
    printf("Unsigned 64-bit fastmod test passed with divisors in interval [%" PRIu64 ", %" PRIu64 "].\n",
           min, max);
  return true;
}
