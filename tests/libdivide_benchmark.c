#include "libdivide.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#if __GNUC__
#define NOINLINE __attribute__((__noinline__))
#else
#define NOINLINE
#endif
 
#define NANOSEC_PER_SEC 1000000000ULL
#define NANOSEC_PER_USEC 1000ULL
#define NANOSEC_PER_MILLISEC 1000000ULL


#ifdef __cplusplus
using namespace libdivide;
#endif

#if defined(_WIN32) || defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN 1
#include <windows.h>
#include <mmsystem.h>
#define LIBDIVIDE_WINDOWS 1
#pragma comment(lib, "winmm")
#endif

#if ! LIBDIVIDE_WINDOWS
#include <sys/time.h> //for gettimeofday()
#endif


struct random_state {
    uint32_t hi;
    uint32_t lo;
};
 
#define SEED {2147483563, 2147483563 ^ 0x49616E42}
#define ITERATIONS (1 << 19)
 
#define GEN_ITERATIONS (1 << 16)
 
uint64_t sGlobalUInt64;
 
static uint32_t my_random(struct random_state *state) {
    state->hi = (state->hi << 16) + (state->hi >> 16);
    state->hi += state->lo;
    state->lo += state->hi;
    return state->hi;
}
 
#if LIBDIVIDE_WINDOWS
static LARGE_INTEGER gPerfCounterFreq;
#endif

#if ! LIBDIVIDE_WINDOWS
static uint64_t nanoseconds(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * NANOSEC_PER_SEC + now.tv_usec * NANOSEC_PER_USEC;
}
#endif

struct FunctionParams_t {
    const void *d; //a pointer to e.g. a uint32_t
    const void *denomPtr; // a pointer to e.g. libdivide_u32_t
    const void *denomBranchfreePtr; // a pointer to e.g. libdivide_u32_t from branchfree
    const void *data; // a pointer to the data to be divided
};

struct time_result {
    uint64_t time;
    uint64_t result;
};

static struct time_result time_function(uint64_t (*func)(struct FunctionParams_t*), struct FunctionParams_t *params) {
    struct time_result tresult;
#if LIBDIVIDE_WINDOWS
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);
	uint64_t result = func(params);
	QueryPerformanceCounter(&end);
	uint64_t diff = end.QuadPart - start.QuadPart;
	sGlobalUInt64 += result;
	tresult.result = result;
	tresult.time = (diff * 1000000000) / gPerfCounterFreq.QuadPart;
#else
    uint64_t start = nanoseconds();
    uint64_t result = func(params);
    uint64_t end = nanoseconds();
    uint64_t diff = end - start;
    sGlobalUInt64 += result;
    tresult.result = result;
    tresult.time = diff;
#endif
	return tresult;
}

//U32
 
NOINLINE
static uint64_t mine_u32(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
        sum += libdivide_u32_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_u32_branchfree(struct FunctionParams_t *params) {
  /* OFK took over, to replace with the LKK algorithm ...
    unsigned iter;
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
        sum += libdivide_u32_branchfree_do(numer, &denom);
    }
    return sum;
  */

  // LKK algo
  unsigned iter;
  const uint32_t d = *(uint32_t *)params->d;
  // should work even for powers of 2 d??
  const uint64_t M = 1 + (0xffffffffffffffff / d);
  
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
	uint32_t quotient = (M* (__uint128_t) numer) >> 64;
        sum +=  quotient;
	/*
	if (quotient != (numer/d))
	  printf("oops got %d not %d d=%d\n",
		 quotient, (numer/d), d);
	*/
    }
    return sum;
}


#if LIBDIVIDE_USE_SSE2
NOINLINE static uint64_t mine_u32_vector(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m128i sumX4 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
}

NOINLINE static uint64_t mine_u32_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m128i sumX4 = _mm_setzero_si128();
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector_alg2(numers, &denom));
        }        
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
}


NOINLINE static uint64_t mine_u32_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m128i sumX4 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_u32_branchfree_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
    
}
#endif

NOINLINE
static uint64_t mine_u32_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg2(numer, &denom);
        }        
    }
 
    return sum;
}
 
NOINLINE
static uint64_t his_u32(struct FunctionParams_t *params) {
    unsigned iter;
    const uint32_t *data = (const uint32_t *)params->data;
    const uint32_t d = *(uint32_t *)params->d;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_u32_generate(struct FunctionParams_t *params) {
    uint32_t *dPtr = (uint32_t *)params->d;
    struct libdivide_u32_t *denomPtr = (struct libdivide_u32_t *)params->denomPtr;
    unsigned iter;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_u32_gen(*dPtr);
    }
    return *dPtr;
}
 
//S32
 
NOINLINE
static uint64_t mine_s32(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int32_t numer = data[iter];
        sum += libdivide_s32_do(numer, &denom);
    }
    return sum;
}


//ofk
int32_t fast_abs(int32_t v) {
  int32_t zero_or_minus1 = v >> 31;
  return  (v ^ zero_or_minus1)-zero_or_minus1;
}


//ofk.  Return (unsigned) the negative of y if x is negative, otherwise y
uint64_t maybe_negate(int32_t x, uint64_t y) {
  int64_t zero_or_minus1 = (x >> 31);  // sign extended
  return (y ^ zero_or_minus1) - zero_or_minus1;
}


//ofk
#define IMUL64HIGH(rh, i1, i2)                                                 \
  do { int64_t unused_output;                                                  \
    asm("imulq %3" : "=d"(rh),"=a"(unused_output) : "a"(i1), "r"(i2) : "cc");} \
  while(0)

//ofk
#define MUL64HIGH(rh, i1, i2)                                                 \
  do { uint64_t unused_output;                                                \
    asm("mulq %3" : "=d"(rh),"=a"(unused_output) : "a"(i1), "r"(i2) : "cc") ;}\
  while(0)


NOINLINE
static uint64_t mine_s32_branchfree(struct FunctionParams_t *params) {

    // OFK hijacked this to be LKK
    /* unsigned iter; */
    /* const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr; */
    /* const int32_t *data = (const int32_t *)params->data; */
    /* int32_t sum = 0; */
    /* for (iter = 0; iter < ITERATIONS; iter++) { */
    /*     int32_t numer = data[iter]; */
    /*     sum += libdivide_s32_branchfree_do(numer, &denom); */
    /* } */
    /* return sum; */

  // unfortunately, the "negate M" trick won't work in all cases.
  // So we can conditionally negate the quotient using M for abs(d)
  // if d is negative.  But we don't normally want to store more than
  // than M. Too bad d=2 uses the MSbit of M...and giving it instead
  // 2^63-1 gets wrong answers.

  // note: this code is broken for d=+1, -1,
  // and -2^31 is probably broken unless we are really lucky
  
  unsigned iter;

  // some of these computations are unnecessary for power-of-2 case
  const int32_t d = *(int32_t *) params->d;
  const uint32_t abs_d = fast_abs(d);
  const uint64_t M1 = 1 + (UINT64_C(0xffffffffffffffff) / abs_d); 
  const uint64_t M = M1 + ((abs_d & (abs_d-1)) == 0); // not needed if specialized code exists for powers of 2.
  const int64_t corrector = d >> 31;

  int sh_amt;
  // is there a quicker bithack to detect pos and neg powers of 2?
  if ((abs_d & (abs_d-1)) == 0) {
    sh_amt = __builtin_ctz(d);
  }
  
      
  const int32_t *data = (const int32_t *)params->data;
  int32_t sum = 0;
  for (iter = 0; iter < ITERATIONS; iter++) {
    int32_t numer = data[iter];

    // Hoped to find that the condition-inside-loop
    // is not transformed into if (condition) loop1 else loop2
    // but it appears that it is.  Furthermore, the power-of-2
    // approach appears to have has been vectorized.   This shows a significant
    // weakness in this benchmark.
    if ((abs_d & (abs_d-1)) == 0) {
#if 0
      // usual power-of-2 approach, cf Hacker's Delight
      // seems to run considerably slower than similar code in libdivide.h
      // uses 3 shifts, 3 add/subs
      const int32_t v1 = numer >> (sh_amt - 1);
      const int32_t v2 =  ( (uint32_t) v1) >> (32 - sh_amt);
      const int32_t v3 = v2 + numer;
      const int32_t quotient1 = v3 >> sh_amt;
#else
      // very similar code stolen from libdivide
      // uses 3 shifts, 2 add/sub and and AND.  But much faster.
      // could investigate the reason with IACA
      const uint32_t uq = (uint32_t)(numer + ((numer >> 31) & ((1U << sh_amt) - 1)));
        int32_t q = (int32_t)uq;
        q = q >> sh_amt;
	int32_t quotient1 = q;
#endif
      // conditional negate, if d is -ve
      int32_t quotient = (quotient1 ^ corrector) - corrector;
      //printf("powerof2 branch\n");
#if 0
      if (quotient != (numer/d))
	printf("oopsA got %d not %d d=%d numer=%d\n",
	       quotient, (numer/d), d, numer);
#endif
      sum += quotient;
    }
    else {
      //printf("non power of 2 branch\n");
      int64_t quotient3;  
      const int64_t numer_corrector = numer >> 31;
      
      // this option is broken for 1,-1, 2, -2, and probably -2^32
      IMUL64HIGH(quotient3, (int64_t) numer, M);
      int32_t quotient2 = quotient3 - numer_corrector;
      
      // negate if necessary
      int32_t quotient = (quotient2 ^ corrector) - corrector;
      
      
      sum +=  quotient;
#if 0   
      if (quotient != (numer/d))
	printf("oops got %d not %d d=%d numer=%d\n",
	       quotient, (numer/d), d, numer);
#endif
    }
  }
  return sum;
}


NOINLINE
static uint64_t mine_s32_branchfree_works(struct FunctionParams_t *params) {

    // OFK hijacked this to be LKK

  // note: this code is broken for d=+1, -1,
  // and -2^63  is probably broken unless we are really lucky
  
  unsigned iter;
  const int32_t d = *(int32_t *) params->d;
  const uint32_t abs_d = fast_abs(d);
  const uint64_t M1 = 1 + (UINT64_C(0xffffffffffffffff) / abs_d); 
  const uint64_t M = M1 + ((abs_d & (abs_d-1)) == 0);
  // unfortunately we may need to store at least one extra bit to record d was -ve
  // so here, we use 64 bits instead...
  // If we knew abs_d >= 3 we could borrow the "sign bit" of M, but we must handle
  // abs_d = 2 also and it already needs the lsb.
  const int64_t corrector = d >> 31;

      
  const int32_t *data = (const int32_t *)params->data;
  int32_t sum = 0;
  for (iter = 0; iter < ITERATIONS; iter++) {
    int32_t numer = data[iter];

    int64_t quotient3;  
    const int64_t numer_corrector = numer >> 31;

#if 0
    // this option is broken for 1,-1, 2, -2, and probably -2^32
    IMUL64HIGH(quotient3, (int64_t) numer, M);
#else
    // more expensive, not broken in the +2, -2 cases but still bad for 1, -1, -2^32
    MUL64HIGH(quotient3, (int64_t) numer, M);
    //quotient3 += M*numer_corrector;  // mimic assembly? not sure why fixup works....
    quotient3 -= (M & numer_corrector);  // should be cheaper and equivalent
#endif
#if 0
    __int128_t M128 = M; // we need to get unsigned extension
    // but then use a signed multiplication
    int64_t quotient1 = ( ((__int128_t) numer) * M128) >> 64; // works but expensive
    if (quotient1 != quotient3 && abs_d > 1) {
      printf("ok Q1 = %ld  Q3 = %ld\n",quotient1, quotient3);
        printf("M is %ld\n",M);
    }
#endif
    
    int32_t quotient2 = quotient3 - numer_corrector;

    // negate if necessary
    int32_t quotient = (quotient2 ^ corrector) - corrector;

      
    sum +=  quotient;
#if 0    
    if (quotient != (numer/d))
      printf("oops got %d not %d d=%d numer=%d\n",
	     quotient, (numer/d), d, numer);
#endif    
  }
  return sum;
}


#if LIBDIVIDE_USE_SSE2
NOINLINE
static uint64_t mine_s32_vector(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX4 = _mm_setzero_si128();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

NOINLINE
static uint64_t mine_s32_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX4 = _mm_setzero_si128();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg1(numers, &denom));
        }                
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg2(numers, &denom));
        }                
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg3(numers, &denom));
        }                
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg4(numers, &denom));
        }                
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

NOINLINE
static uint64_t mine_s32_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX4 = _mm_setzero_si128();
    const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_s32_branchfree_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

#endif

NOINLINE
static uint64_t mine_s32_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    int32_t sum = 0;
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg2(numer, &denom);
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg3(numer, &denom);
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg4(numer, &denom);
        }        
    }
    
    return (uint64_t)sum;
}

NOINLINE
static uint64_t his_s32(struct FunctionParams_t *params) {
    unsigned iter;
    int32_t sum = 0;
    const int32_t d = *(int32_t *)params->d;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int32_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_s32_generate(struct FunctionParams_t *params) {
    unsigned iter;
    int32_t *dPtr = (int32_t *)params->d;
    struct libdivide_s32_t *denomPtr = (struct libdivide_s32_t *)params->denomPtr;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_s32_gen(*dPtr);
    }
    return *dPtr;
}
 
 
//U64
 
NOINLINE
static uint64_t mine_u64(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    uint64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint64_t numer = data[iter];
        sum += libdivide_u64_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_u64_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    uint64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint64_t numer = data[iter];
        sum += libdivide_u64_branchfree_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_u64_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    uint64_t sum = 0;
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg2(numer, &denom);
        }        
    }
    
    return sum;
}

#if LIBDIVIDE_USE_SSE2
NOINLINE static uint64_t mine_u64_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector_alg0(numers, &denom));
        }   
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector_alg1(numers, &denom));
        }   
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector_alg2(numers, &denom));
        }        
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}

NOINLINE static uint64_t mine_u64_vector(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}

NOINLINE static uint64_t mine_u64_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_u64_branchfree_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}
#endif

NOINLINE
static uint64_t his_u64(struct FunctionParams_t *params) {
    unsigned iter;
    uint64_t sum = 0;
    const uint64_t d = *(uint64_t *)params->d;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint64_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_u64_generate(struct FunctionParams_t *params) {
    unsigned iter;
    uint64_t *dPtr = (uint64_t *)params->d;
    struct libdivide_u64_t *denomPtr = (struct libdivide_u64_t *)params->denomPtr;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_u64_gen(*dPtr);
    }
    return *dPtr;
}

NOINLINE
static uint64_t mine_s64(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int64_t numer = data[iter];
        sum += libdivide_s64_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_s64_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int64_t numer = data[iter];
        sum += libdivide_s64_branchfree_do(numer, &denom);
    }
    return sum;
}


#if LIBDIVIDE_USE_SSE2
NOINLINE
static uint64_t mine_s64_vector(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}

NOINLINE
static uint64_t mine_s64_vector_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_s64_branchfree_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}

NOINLINE
static uint64_t mine_s64_vector_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    int algo = libdivide_s64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg2(numers, &denom));
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg3(numers, &denom));
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg4(numers, &denom));
        }        
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}
#endif

NOINLINE
static uint64_t mine_s64_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;

    unsigned iter;
    int64_t sum = 0;
    int algo = libdivide_s64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg2(numer, &denom);
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg3(numer, &denom);
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg4(numer, &denom);
        }        
    }
    
    return sum;
}
 
NOINLINE
static uint64_t his_s64(struct FunctionParams_t *params) {
    const int64_t *data = (const int64_t *)params->data;
    const int64_t d = *(int64_t *)params->d;

    unsigned iter;
    int64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int64_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_s64_generate(struct FunctionParams_t *params) {
    int64_t *dPtr = (int64_t *)params->d;
    struct libdivide_s64_t *denomPtr = (struct libdivide_s64_t *)params->denomPtr;
    unsigned iter;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_s64_gen(*dPtr);
    }
    return *dPtr;
}

/* Stub functions for when we have no SSE2 */
#if ! LIBDIVIDE_USE_SSE2
NOINLINE static uint64_t mine_u32_vector(struct FunctionParams_t *params) { return mine_u32(params); }
NOINLINE static uint64_t mine_u32_vector_unswitched(struct FunctionParams_t *params) { return mine_u32_unswitched(params); }
NOINLINE static uint64_t mine_u32_vector_branchfree(struct FunctionParams_t *params) { return mine_u32_branchfree(params); }
NOINLINE static uint64_t mine_s32_vector(struct FunctionParams_t *params) { return mine_s32(params); }
NOINLINE static uint64_t mine_s32_vector_unswitched(struct FunctionParams_t *params) { return mine_s32_unswitched(params); }
NOINLINE static uint64_t mine_s32_vector_branchfree(struct FunctionParams_t *params) { return mine_s32_branchfree(params); }
NOINLINE static uint64_t mine_u64_vector(struct FunctionParams_t *params) { return mine_u64(params); }
NOINLINE static uint64_t mine_u64_vector_unswitched(struct FunctionParams_t *params) { return mine_u64_unswitched(params); }
NOINLINE static uint64_t mine_u64_vector_branchfree(struct FunctionParams_t *params) { return mine_u64_branchfree(params); }
NOINLINE static uint64_t mine_s64_vector(struct FunctionParams_t *params) { return mine_s64(params); }
NOINLINE static uint64_t mine_s64_vector_unswitched(struct FunctionParams_t *params) { return mine_s64_unswitched(params); }
NOINLINE static uint64_t mine_s64_vector_branchfree(struct FunctionParams_t *params) { return mine_s64_branchfree(params); }
#endif
 
struct TestResult {
    double my_base_time;
    double my_branchfree_time;
    double my_unswitched_time;
    double my_vector_time;
    double my_vector_branchfree_time;
    double my_vector_unswitched_time;
    double his_time;
    double gen_time;
    int algo;
};
 
static uint64_t find_min(const uint64_t *vals, size_t cnt) {
    uint64_t result = vals[0];
    size_t i;
    for (i=1; i < cnt; i++) {
        if (vals[i] < result) result = vals[i];
    }
    return result;
}
 
typedef uint64_t (*TestFunc_t)(struct FunctionParams_t *params);
 
NOINLINE
struct TestResult test_one(TestFunc_t mine, TestFunc_t mine_branchfree, TestFunc_t mine_vector, TestFunc_t mine_unswitched, TestFunc_t mine_vector_unswitched, TestFunc_t mine_vector_branchfree, TestFunc_t his, TestFunc_t generate, struct FunctionParams_t *params) {
#define TEST_COUNT 30
    struct TestResult result;
    memset(&result, 0, sizeof result);
    
#define CHECK(actual, expected) do { if (1 && actual != expected) printf("Failure on line %lu\n", (unsigned long)__LINE__); } while (0)
    
    uint64_t my_times[TEST_COUNT], my_times_branchfree[TEST_COUNT], my_times_unswitched[TEST_COUNT], my_times_vector[TEST_COUNT], my_times_vector_unswitched[TEST_COUNT], my_times_vector_branchfree[TEST_COUNT], his_times[TEST_COUNT], gen_times[TEST_COUNT];
    unsigned iter;
    struct time_result tresult;
    for (iter = 0; iter < TEST_COUNT; iter++) {
        tresult = time_function(his, params); his_times[iter] = tresult.time; const uint64_t expected = tresult.result;
        tresult = time_function(mine, params); my_times[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_branchfree, params); my_times_branchfree[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_unswitched, params); my_times_unswitched[iter] = tresult.time; CHECK(tresult.result, expected);
#if LIBDIVIDE_USE_SSE2
        tresult = time_function(mine_vector, params); my_times_vector[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_vector_branchfree, params); my_times_vector_branchfree[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_vector_unswitched, params); my_times_vector_unswitched[iter] = tresult.time; CHECK(tresult.result, expected);
#else
		my_times_vector[iter]=0;
		my_times_vector_unswitched[iter] = 0;
        my_times_vector_branchfree[iter] = 0;
#endif
        tresult = time_function(generate, params); gen_times[iter] = tresult.time;
    }
        
    result.gen_time = find_min(gen_times, TEST_COUNT) / (double)GEN_ITERATIONS;
    result.my_base_time = find_min(my_times, TEST_COUNT) / (double)ITERATIONS;
    result.my_branchfree_time = find_min(my_times_branchfree, TEST_COUNT) / (double)ITERATIONS;
    result.my_vector_time = find_min(my_times_vector, TEST_COUNT) / (double)ITERATIONS;
	//printf("%f - %f\n", find_min(my_times_vector, TEST_COUNT) / (double)ITERATIONS, result.my_vector_time);
    result.my_unswitched_time = find_min(my_times_unswitched, TEST_COUNT) / (double)ITERATIONS;
    result.my_vector_branchfree_time = find_min(my_times_vector_branchfree, TEST_COUNT) / (double)ITERATIONS;
    result.my_vector_unswitched_time = find_min(my_times_vector_unswitched, TEST_COUNT) / (double)ITERATIONS;
    result.his_time = find_min(his_times, TEST_COUNT) / (double)ITERATIONS;
    return result;
#undef TEST_COUNT
}
 
NOINLINE
struct TestResult test_one_u32(uint32_t d, const uint32_t *data) {
    int no_branchfree = (d == 1);
    struct libdivide_u32_t div_struct = libdivide_u32_gen(d);
    struct libdivide_u32_branchfree_t div_struct_bf = libdivide_u32_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = &div_struct_bf;
    params.data = data;
        
    struct TestResult result = test_one(mine_u32,
                                        no_branchfree ? mine_u32 : mine_u32_branchfree,
                                        mine_u32_vector,
                                        mine_u32_unswitched,
                                        mine_u32_vector_unswitched,
                                        no_branchfree ? mine_u32_vector : mine_u32_vector_branchfree,
                                        his_u32,
                                        mine_u32_generate,
                                        &params);
    result.algo = libdivide_u32_get_algorithm(&div_struct);
    return result;
}
 
NOINLINE
struct TestResult test_one_s32(int32_t d, const int32_t *data) {
    int no_branchfree = (d == 1 || d == -1);
    struct libdivide_s32_t div_struct = libdivide_s32_gen(d);
    struct libdivide_s32_branchfree_t div_struct_bf = libdivide_s32_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = &div_struct_bf;
    params.data = data;
    
    struct TestResult result = test_one(mine_s32,
                                        no_branchfree ? mine_s32 : mine_s32_branchfree,
                                        mine_s32_vector,
                                        mine_s32_unswitched,
                                        mine_s32_vector_unswitched,
                                        no_branchfree ? mine_s32_vector : mine_s32_vector_branchfree,
                                        his_s32,
                                        mine_s32_generate,
                                        &params);
    result.algo = libdivide_s32_get_algorithm(&div_struct);
    return result;
}
 
NOINLINE
struct TestResult test_one_u64(uint64_t d, const uint64_t *data) {
    int no_branchfree = (d == 1);
    struct libdivide_u64_t div_struct = libdivide_u64_gen(d);
    struct libdivide_u64_branchfree_t div_struct_bf = libdivide_u64_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = &div_struct_bf;
    params.data = data;
    
    struct TestResult result = test_one(mine_u64,
                                        no_branchfree ? mine_u64 : mine_u64_branchfree,
                                        mine_u64_vector,
                                        mine_u64_unswitched,
                                        mine_u64_vector_unswitched,
                                        no_branchfree ? mine_u64_vector : mine_u64_vector_branchfree,
                                        his_u64,
                                        mine_u64_generate,
                                        &params);
    result.algo = libdivide_u64_get_algorithm(&div_struct);
    return result;
}
 
NOINLINE
struct TestResult test_one_s64(int64_t d, const int64_t *data) {
    int no_branchfree = (d == 1 || d == -1);
    struct libdivide_s64_t div_struct = libdivide_s64_gen(d);
    struct libdivide_s64_branchfree_t div_struct_bf = libdivide_s64_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = no_branchfree ? (void *)&div_struct : (void *)&div_struct_bf;
    params.data = data;
    
    struct TestResult result = test_one(mine_s64, no_branchfree ? mine_s64 : mine_s64_branchfree, mine_s64_vector, mine_s64_unswitched, mine_s64_vector_unswitched, mine_s64_vector_branchfree, his_s64, mine_s64_generate, &params);
    result.algo = libdivide_s64_get_algorithm(&div_struct);
    return result;
}
 
 
static void report_header(void) {
  // mod by OFK, was scl_bf not scl_LKK
    printf("%6s%8s%8s%8s%8s%8s%8s%8s%8s%6s\n", "#", "system", "scalar", "scl_LKK", "scl_us", "vector", "vec_bf", "vec_us", "gener", "algo");
}
 
static void report_result(const char *input, struct TestResult result) {
    printf("%6s%8.3f%8.3f%8.3f%8.3f%8.3f%8.3f%8.3f%8.3f%6d\n", input, result.his_time, result.my_base_time, result.my_branchfree_time, result.my_unswitched_time, result.my_vector_time, result.my_vector_branchfree_time, result.my_vector_unswitched_time, result.gen_time, result.algo);
}
 
static void test_many_u32(const uint32_t *data) {
    report_header();
    uint32_t d;
    for (d=1; d > 0; d++) {
        struct TestResult result = test_one_u32(d, data);
        char input_buff[32];
        sprintf(input_buff, "%u", d);
        report_result(input_buff, result);
    }
}
 
static void test_many_s32(const int32_t *data) {
    report_header();
    int32_t d;
    for (d=1; d != 0;) {
        struct TestResult result = test_one_s32(d, data);
        char input_buff[32];
        sprintf(input_buff, "%d", d);
        report_result(input_buff, result);
        
        d = -d;
        if (d > 0) d++;
    }
}
 
static void test_many_u64(const uint64_t *data) {
    report_header();
    uint64_t d;
    for (d=1; d > 0; d++) {
        struct TestResult result = test_one_u64(d, data);
        char input_buff[32];
        sprintf(input_buff, "%" PRIu64, d);
        report_result(input_buff, result);
    }    
}
 
static void test_many_s64(const int64_t *data) {
    report_header();
    int64_t d;
    for (d=1; d != 0;) {
        struct TestResult result = test_one_s64(d, data);
        char input_buff[32];
        sprintf(input_buff, "%" PRId64, d);
        report_result(input_buff, result);
        
        d = -d;
        if (d > 0) d++;
    }
}
 
static const uint32_t *random_data(unsigned multiple) {
#if LIBDIVIDE_WINDOWS
    uint32_t *data = (uint32_t *)malloc(multiple * ITERATIONS * sizeof *data);
#else
    /* Linux doesn't always give us data sufficiently aligned for SSE, so we can't use malloc(). */
    void *ptr = NULL;
    posix_memalign(&ptr, 16, multiple * ITERATIONS * sizeof(uint32_t));
    uint32_t *data = ptr;
#endif
    uint32_t i;
    struct random_state state = SEED;
    for (i=0; i < ITERATIONS * multiple; i++) {
        data[i] = my_random(&state);
    }
    return data;
}


int main(int argc, char* argv[]) {
#if LIBDIVIDE_WINDOWS
	QueryPerformanceFrequency(&gPerfCounterFreq);
#endif
    int i, u32 = 0, u64 = 0, s32 = 0, s64 = 0;

    printf("This version has been modified so that branchfree replaced by LKK\n");

    if (argc == 1) {
        /* Test all */
        u32 = u64 = s32 = s64 = 1;
        u64 = s64 = 0;  // OFK
    }
    else {
        for (i=1; i < argc; i++) {
            if (! strcmp(argv[i], "u32")) u32 = 1;
            else if (! strcmp(argv[i], "u64")) printf("ignoring 64-bit request"); // OFK, was u64 = 1;
            else if (! strcmp(argv[i], "s32")) s32 = 1;
            else if (! strcmp(argv[i], "s64")) printf("ignoring 64-bit request"); // OFK, was s64 = 1;
            else printf("Unknown test '%s'\n", argv[i]), exit(0);
        }
    }
    const uint32_t *data = NULL;
    data = random_data(1);
    if (u32) test_many_u32(data);
    if (s32) test_many_s32((const int32_t *)data);
    free((void *)data);
    
    data = random_data(2);
    if (u64) test_many_u64((const uint64_t *)data);
    if (s64) test_many_s64((const int64_t *)data);
    free((void *)data);
    return 0;
}
