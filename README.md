# fastmod
[![Build Status](https://travis-ci.org/lemire/fastmod.svg?branch=master)](https://travis-ci.org/lemire/fastmod)

A header file for fast 32-bit division remainders  on 64-bit hardware.

How fast? Faster than your compiler can do it!

Compilers cleverly replace divisions by multiplications and shifts, if the divisor is known at compile time. In a hashing benchmark, our simple C code can beat  state-of-the-art compilers (e.g., LLVM clang, GNU GCC) on a recent Intel processor (Skylake).

<img src="docs/hashbenches-skylake-clang.png" width="90%">

Further reading:

- [Faster Remainder by Direct Computation: Applications to Compilers and Software Libraries](https://arxiv.org/abs/1902.01961), Software: Practice and Experience (to appear)


##  Usage

We support all major compilers (LLVM's clang, GNU GCC, Visual Studio). Users of Visual Studio need to compile to a 64-bit binary, typically by selecting x64 in the build settings.

It is a header-only library but we have unit tests. Assuming a Linux/macOS setting:

```
make
./unit
```

The tests are exhaustive and take some time.

##  Code samples

In C, you can use the header as follows.

```C
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

In C++, it is much the same except that every function is in the `fastmod` namespace so you need to prefix the calls with `fastmod::` (e.g., `fastmod::is_divisible`).


### 64-bit benchmark

For comparisons to native `%` and `/` operations, as well as bitmasks, we've provided a benchmark with 64-bit div/mod. You can compile these benchmarks with `make benchmark`.
These require C++11. Example output:

```
$ make benchmark
g++-8 -fPIC  -std=c++11 -O3 -march=native -Wall -Wextra -Wshadow -o modnbenchmark tests/modnbenchmark.cpp -Iinclude
g++-8 -fPIC  -std=c++11 -O3 -march=native -Wall -Wextra -Wshadow -o moddivnbenchmark tests/moddivnbenchmark.cpp -Iinclude
$ ./modnbenchmark && ./moddivnbenchmark 
Time: 176997000
Time: 819993000
Time: 80945000
masking is 2.186633 as fast as fastmod and 10.130249 as fast as modding
Time: 32193000
Time: 81158000
fastmod + fastdiv is 2.520983 as fast as x86 mod + div: 
```


## Go version

* There is a Go version of this library: https://github.com/bmkessler/fastdiv

