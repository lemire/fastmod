# fastmod
[![Build Status](https://travis-ci.org/lemire/fastmod.svg?branch=master)](https://travis-ci.org/lemire/fastmod)

A header file for fast 32-bit division remainders  on 64-bit hardware.

How fast? Faster than your compiler can do it!

Compilers cleverly replace divisions by multiplications and shifts, if the divisor is known at compile time. In a hashing benchmark, our simple C code can beat  state-of-the-art compilers (e.g., LLVM clang, GNU GCC) on a recent Intel processor (Skylake).

<img src="docs/hashbenches-skylake-clang.png" width="90%">

Further reading:

- [Faster Remainder by Direct Computation: Applications to Compilers and Software Libraries](https://arxiv.org/abs/1902.01961), Software: Practice and Experience (to appear)


##  Usage

We support all major compilers (LLVM's clang, GNU GCC, Visual Studio). Users of Visual Studio need to compile to a 64-bit binary (x64).

It is a header-only library but we have unit tests. Assuming a Linux/macOS setting:

```
make
./unit
```

The tests are exhaustive and take some time.

##  Code samples

```
#include "fastmod.h"

// unsigned...

uint32_t d = ... ; // divisor, should be non-zero
uint64_t M = computeM_u32(d); // do once

fastmod_u32(a,M,d) is a % d for all 32-bit unsigned values a.

is_divisible(a,M) tells you if a is divisible by d

// signed...

int32_t d = ... ; // should be non-zero and between [-2147483647,2147483647]
int32_t positive_d = d < 0 ? -d : d; // absolute value
uint64_t M = computeM_s32(d); // do once

fastmod_s32(a,M,positive_d) is a % d for all 32-bit a
```
