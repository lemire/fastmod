#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "fastmod.h"

using namespace fastmod;


bool testunsigned64(uint64_t min, uint64_t max, bool verbose);



bool testunsigned(uint32_t min, uint32_t max, bool verbose) {
  for (uint32_t d = min; (d <= max) && (d >= min); d++) {
    if (d == 0) {
      printf("skipping d = 0\n");
      continue;
    }
    uint64_t M = computeM_u32(d);
    uint32_t computedMod = 0;
    if (verbose)
      printf("d = %u (unsigned) ", d);
    else
      printf(".");
    fflush(NULL);
    for (uint64_t a64 = 0; a64 < UINT64_C(0x100000000); a64++) {
      uint32_t a = (uint32_t)a64;
      uint32_t computedFastMod = fastmod_u32(a, M, d);
      if (computedMod != computedFastMod) {
        printf(
            "(bad unsigned fastmod) problem with divisor %u and dividend %u \n",
            d, a);
        printf("expected %u mod %u = %u \n", a, d, computedMod);
        printf("got %u mod %u = %u \n", a, d, computedFastMod);
        return false;
      }
      if ((computedMod == 0) != is_divisible(a, M)) {
        printf("problem with divisibility test. divisor %u and dividend %u \n",
               d, a);
        return false;
      }
      computedMod++; // presumably, this is faster than computedMod = a % d
      if (computedMod == d)
        computedMod = 0;
    }
    if (verbose)
      printf("ok!\n");
  }
  if (verbose)
    printf("Unsigned fastmod test passed with divisors in interval [%u, %u].\n",
           min, max);
  return true;
}

bool testdivunsigned(uint32_t min, uint32_t max, bool verbose) {
  for (uint32_t d = min; (d <= max) && (d >= min); d++) {
    if (d == 0) {
      printf("skipping d = 0\n");
      continue;
    }
    if (d == 1) {
      printf("skipping d = 1 as it is not supported\n");
      continue;
    }
    uint64_t M = computeM_u32(d);
    if (verbose)
      printf("d = %u (unsigned) ", d);
    else
      printf(".");
    fflush(NULL);
    for (uint64_t a64 = 0; a64 < UINT64_C(0x100000000); a64++) {
      uint32_t a = (uint32_t)a64;
      uint32_t computedDiv = a / d;

      uint32_t computedFastDiv = fastdiv_u32(a, M);
      if (computedDiv != computedFastDiv) {
        printf(
            "(bad unsigned fastdiv) problem with divisor %u and dividend %u \n",
            d, a);
        printf("expected %u div %u = %u \n", a, d, computedDiv);
        printf("got %u div %u = %u \n", a, d, computedFastDiv);
        return false;
      }
    }
    if (verbose)
      printf("ok!\n");
  }
  if (verbose)
    printf("Unsigned fastdiv test passed with divisors in interval [%u, %u].\n",
           min, max);
  return true;
}

bool testsigned(int32_t min, int32_t max, bool verbose) {
  assert(min != max + 1); // infinite loop!
  for (int32_t d = min; (d <= max) && (d >= min); d++) {
    if (d == 0) {
      printf("skipping d = 0 as it cannot be supported\n");
      continue;
    }
    if (d == -2147483648) {
      printf("skipping d = -2147483648 as it is unsupported\n");
      continue;
    }
    uint64_t M = computeM_s32(d);
    if (verbose)
      printf("d = %d (signed) ", d);
    else
      printf(".");
    fflush(NULL);
    int32_t positive_d = d < 0 ? -d : d;
    uint64_t absolute_min32 = -INT64_C(0x80000000);
    if (d == -1)
      absolute_min32 += 1; // otherwise, result is undefined
    for (int64_t a64 = absolute_min32; a64 < INT64_C(0x80000000); a64++) {
      int32_t a = (int32_t)a64;
      int32_t computedMod = a % d;
      int32_t computedFastMod = fastmod_s32(a, M, positive_d);
      if (computedMod != computedFastMod) {
        printf(
            "(bad signed fastmod) problem with divisor %d and dividend %d \n",
            d, a);
        printf("expected %d mod %d = %d \n", a, d, computedMod);
        printf("got %d mod %d = %d \n", a, d, computedFastMod);
        return false;
      }
    }
    if (verbose)
      printf("ok!\n");
  }
  if (verbose)
    printf("Signed fastmod test passed with divisors in interval [%d, %d].\n",
           min, max);
  return true;
}

bool testdivsigned(int32_t min, int32_t max, bool verbose) {
  assert(min != max + 1); // infinite loop!
  for (int32_t d = min; (d <= max) && (d >= min); d++) {
    if (d == 0) {
      printf("skipping d = 0\n");
      continue;
    }
    if (d == 1) {
      printf("skipping d = 1 as it is not supported\n");
      continue;
    }
    if (d == -1) {
      printf("skipping d = -1 as it is not supported\n");
      continue;
    }
    if (d == -2147483648) {
      printf("skipping d = -2147483648 as it is unsupported\n");
      continue;
    }

    uint64_t M = computeM_s32(d);
    if (verbose)
      printf("d = %d (signed) ", d);
    else
      printf(".");
    fflush(NULL);
    for (int64_t a64 = -INT64_C(0x80000000); a64 < INT64_C(0x80000000); a64++) {
      int32_t a = (int32_t)a64;
      int32_t computedDiv = a / d;
      int32_t computedFastDiv = fastdiv_s32(a, M, d);
      if (computedDiv != computedFastDiv) {
        printf(
            "(bad signed fastdiv) problem with divisor %d and dividend %d \n",
            d, a);
        printf("expected %d div %d = %d \n", a, d, computedDiv);
        printf("got %d div %d = %d \n", a, d, computedFastDiv);
        return false;
      }
    }
    if (verbose)
      printf("ok!\n");
  }
  if (verbose)
    printf("Signed fastdiv test passed with divisors in interval [%d, %d].\n",
           min, max);
  return true;
}



int main(int argc, char *argv[]) {
  bool isok = true;
  bool verbose = false;
  for(int i = 0; i < argc; ++i) {
    if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
        verbose = true; break;
    }
  }

  isok = isok && testunsigned64(1, 0x10, verbose);
  isok = isok && testunsigned64(0x000133F, 0xFFFF, verbose);
  isok = isok && testunsigned64(UINT64_C(0xffffffffff00000), UINT64_C(0xffffffffff00000) + 0x100, verbose);
  isok = isok && testunsigned(1, 8, verbose);
  isok = isok && testunsigned(0xfffffff8, 0xffffffff, verbose);
  isok = isok && testdivsigned(-0x80000000, -0x7ffffff8, verbose);
  isok = isok && testdivsigned(2, 10, verbose);
  isok = isok && testdivsigned(0x7ffffff8, 0x7fffffff, verbose);
  isok = isok && testdivsigned(-10, -2, verbose);

  isok = isok && testdivunsigned(2, 10, verbose);
  isok = isok && testdivunsigned(0xfffffff8, 0xffffffff, verbose);

  isok = isok && testsigned(-8, -1, verbose);
  isok = isok && testsigned(1, 8, verbose);
  isok = isok && testsigned(0x7ffffff8, 0x7fffffff, verbose);
  isok = isok && testsigned(-0x80000000, -0x7ffffff8, verbose);
  for (int k = 0; k < 100; k++) {
    int32_t x = rand();
    isok = isok && testsigned(x, x, verbose);
    if (x < 0) {
      isok = isok && testunsigned(-x, -x, verbose);
      isok = isok && testunsigned(-x, -x, verbose);
    } else {
      isok = isok && testunsigned(x, x, verbose);
    }
  }
  printf("\n");
  if (isok) {
    printf("Code looks good.\n");
    return 0;
  } else {
    printf("You have some failing tests.\n");
    return -1;
  }
}
