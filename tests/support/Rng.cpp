#include "Rng.h"

Rng::Rng(uint64_t seed) : state(seed) {}

uint64_t Rng::nextU64() {
  state += 0x9E3779B97f4A7C15ULL;
  uint64_t z = state;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

int Rng::nextInt(int lo, int hi) {
  uint64_t range = static_cast<uint64_t>(hi - lo + 1);
  return lo + static_cast<int>(nextU64() % range);
}

double Rng::nextDouble() {
  // Take the top 53 bits so every double in [0,1) is reachable evenly.
  return static_cast<double>(nextU64() >> 11) * (1.0 / 9007199254740992.0);
}

bool Rng::nextBool(double trueProbability) {
  return nextDouble() < trueProbability;
}
