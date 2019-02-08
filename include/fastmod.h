#ifndef FASTMOD_H
#define FASTMOD_H

#include <stdint.h>
#include <stdbool.h>



/**
* Unsigned integers.
* Usage:
*  uint32_t d = ... ; // divisor, should be non-zero
*  uint64_t M = computeM_u32(d); // do once
*  fastmod_u32(a,M,d) is a % d for all 32-bit a.
*
**/

// M = ceil( (1<<64) / d ), d > 0
static inline uint64_t computeM_u32(uint32_t d) {
  return UINT64_C(0xFFFFFFFFFFFFFFFF) / d + 1;
}

// fastmod computes (a % d) given precomputed M
static inline uint32_t fastmod_u32(uint32_t a, uint64_t M, uint32_t d) {
  uint64_t lowbits = M * a;
  return ((__uint128_t)lowbits * d) >> 64;
}

// fastmod computes (a / d) given precomputed M for d>1
static inline uint32_t fastdiv_u32(uint32_t a, uint64_t M) {
  return ( (__uint128_t) M * a ) >> 64;
}

// given precomputed M, checks whether n % d == 0
static inline bool is_divisible(uint32_t n, uint64_t M) {
  return n * M <= M - 1;
}
/**
* signed integers
* Usage:
*  int32_t d = ... ; // should be non-zero and between [-2147483647,2147483647]
*  int32_t positive_d = d < 0 ? -d : d; // absolute value
*  uint64_t M = computeM_s32(d); // do once
*  fastmod_s32(a,M,positive_d) is a % d for all 32-bit a.
**/

// M = floor( (1<<64) / d ) + 1
// you must have that d is different from 0 and -2147483648
// if d = -1 and a = -2147483648, the result is undefined
static inline uint64_t computeM_s32(int32_t d) {
  if (d < 0)
    d = -d;
  return UINT64_C(0xFFFFFFFFFFFFFFFF) / d + 1 + ((d & (d - 1)) == 0 ? 1 : 0);
}

// fastmod computes (a % d) given precomputed M,
// you should pass the absolute value of d
static inline int32_t fastmod_s32(int32_t a, uint64_t M, int32_t positive_d) {
  uint64_t lowbits = M * a;
  int32_t highbits = ((__uint128_t)lowbits * positive_d) >> 64;
  return highbits - ((positive_d - 1) & (a >> 31));
}

// fastmod computes (a / d) given precomputed M, assumes that d must not
// be one of -1, 1, or -2147483648
static inline int32_t fastdiv_s32(int32_t a, uint64_t M, int32_t d) {
  uint64_t highbits = ((__uint128_t) M * a) >> 64;
  highbits += (a < 0 ? 1 : 0);
  if(d < 0) return -highbits;
  return highbits;
}


#endif
