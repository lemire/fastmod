#include "fastmod.h"
#include <cstdio>
#include <chrono>
#include <random>
#include <iostream>


template <typename T> inline void doNotOptimizeAway(T&& datum) {
    // Taken from Facebook's folly https://github.com/facebook/folly/blob/0f6bc7a3f0133bd49226b50026de60e708900577/folly/Benchmark.h#L318-L326
     asm volatile("" ::"m"(datum) : "memory");
}

using namespace fastmod;
template<typename F>
uint64_t time(const F &x, std::vector<uint64_t> &zomg) {
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto e: zomg) {
        doNotOptimizeAway(x(e));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = end - start;
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();
    std::fprintf(stderr, "Time: %zu\n", size_t(ns));
    return ns;
}

int main() {
    std::mt19937_64 mt;
    uint64_t mod = mt() % (1 << 27);
    uint64_t constantmod = 34124121;
    std::vector<uint64_t> zomg(100000000);
    for(auto &e: zomg) e = mt();
    const auto M = computeM_u64(mod);
    std::cout << "timing fastmod_u64 " << std::endl;
    auto fmtime = time([M,mod](uint64_t v) {return fastmod_u64(v, M, mod);}, zomg);
    std::cout << "timing x modulo mod; " << std::endl;
    auto modtime = time([mod](uint64_t x) {return x % mod;}, zomg);

    std::cout << "timing fastmod_u64 constantmod" << std::endl;
    const auto constantM = computeM_u64(constantmod);
    auto cfmtime = time([constantM,constantmod](uint64_t v) {return fastmod_u64(v, constantM, constantmod);}, zomg);
    std::cout << "timing x modulo constantmod; " << std::endl;
    auto cmodtime = time([constantmod](uint64_t x) {return x % constantmod;}, zomg);
    uint64_t t = mod - 1;
    t |= t >> 1;
    t |= t >> 2;
    t |= t >> 4;
    t |= t >> 8;
    t |= t >> 16;
    t |= t >> 32; // t is now a bitmask for the rounded up size
    std::cout << "timing x & t; " << std::endl;
    auto masktime = time([t](uint64_t x) {return x & t;}, zomg);
    std::fprintf(stderr, "masking is %lf as fast as fastmod and %lf as fast as modding\n", (double)fmtime / masktime, (double)modtime / masktime);
}
