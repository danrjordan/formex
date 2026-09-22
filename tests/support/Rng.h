#pragma once
#include <cstdint>

// Minimal deterministic PRNG (splitmix64). We don't use
// std::uniform_int_distribution / std::mt19937 here because the standard
// does not guarantee identical output across standard-library
// implementations for a given seed -- only the engine's raw output stream
// is specified, not how distributions map it to a range. splitmix64 is
// pure integer arithmetic, so a given seed produces the same sequence on
// any platform/compiler.
class Rng {
public:
  explicit Rng(uint64_t seed);

  uint64_t nextU64();
  int nextInt(int lo, int hi);       // inclusive on both ends
  double nextDouble();               // [0, 1)
  bool nextBool(double trueProbability);

private:
  uint64_t state;
};
