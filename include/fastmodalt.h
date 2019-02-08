#ifndef FASTMODALT_H
#define FASTMODALT_H

// collection of alternative functions to those offered in fastmod.h
// they may be faster or more convenient.


#include <stdint.h>
#include <stdbool.h>

// could be faster than   "return (v<0) ? -v : v" on some systems
static inline int32_t fast_abs(int32_t v) {
  int32_t zero_or_minus1 = v >> 31;
  return  (v ^ zero_or_minus1)-zero_or_minus1;
}

static inline bool is_positive_power_of_two(int32_t d) {
  return (d & (d-1)) == 0;
}



#define IMUL64HIGH(rh, i1, i2)                                                 \
  do { int64_t unused_output;                                                  \
    asm("imulq %3" : "=d"(rh),"=a"(unused_output) : "a"(i1), "r"(i2) : "cc");} \
  while(0)

#define MUL64HIGH(rh, i1, i2)                                                 \
  do { int64_t unused_output;                                                  \
    asm("mulq %3" : "=d"(rh),"=a"(unused_output) : "a"(i1), "r"(i2) : "cc");} \
  while(0)



// use with computeM_s32
// this option is broken for 1,-1, 2, -2, and -2**31
static inline int32_t fastdiv_s32_ofk_special(int32_t a, uint64_t M, int32_t d) {

  int64_t quotient3;
  const int64_t corrector = d >> 31;
  const int64_t numer_corrector = a >> 31;

  IMUL64HIGH(quotient3, (int64_t) a, M);
  int32_t quotient2 = quotient3 - numer_corrector;

  // negate if necessary
  int32_t quotient = (quotient2 ^ corrector) - corrector;
  return quotient;
}


// broken for +-1 also minor breakage d= -2**31 since d/d gives 0
static inline int32_t fastdiv_s32_ofk2(int32_t a, uint64_t M, int32_t d) {

  int64_t quotient3;
  const int64_t corrector = d >> 31;
  const int64_t numer_corrector = a >> 31;

  // based on the instructions used with unsigned 128bit multiplication
  MUL64HIGH(quotient3, (int64_t) a, M);
  quotient3 -= (M & numer_corrector);  // except should be cheaper and equivalent

  int32_t quotient2 = quotient3 - numer_corrector;

  // negate if necessary
  int32_t quotient = (quotient2 ^ corrector) - corrector;
  return quotient;
}

static inline uint64_t computeM_s32_ofk(int32_t d) {
  const uint32_t abs_d = fast_abs(d);
  if (is_positive_power_of_two(abs_d)) {
    return __builtin_ctz(d);
  }
  else {
    const uint64_t M = 1 + (UINT64_C(0xffffffffffffffff) / abs_d);
    return M;
  }
}

//  work for all nonzero d
static inline int32_t fastdiv_s32_ofk(int32_t a, uint64_t M, int32_t d) {
  const int64_t corrector = d >> 31;
  int32_t quotient;

  if (is_positive_power_of_two(fast_abs(d))) {
      // code from libdivide
      // uses 3 shifts, 2 add/sub and and AND.  But much faster than the usual
      // sequence from Warren (same number of instructions).
      // M was co-opted to be a shift amount for d a power of 2
      const uint32_t uq = (uint32_t)(a + ((a >> 31) & ((1U << M) -1)));
      quotient = (int32_t) uq;
      quotient = quotient >> M;
  }
  else {
    int64_t quotient3;
    const int64_t numer_corrector = a >> 31;

    IMUL64HIGH(quotient3, (int64_t) a, M);
    quotient = quotient3 - numer_corrector;
  }
  // negate if necessary
  quotient = (quotient ^ corrector) - corrector;
  return quotient;
}


#endif
