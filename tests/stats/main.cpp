#include "ExprGen.h"
#include "NodeCount.h"
#include "Rng.h"
#include "TreeEqual.h"
#include "formex/Differentiator.h"
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr uint64_t kSeed = 1337;
constexpr int kSampleCount = 500;
constexpr ExprGenConfig kGenConfig{}; // defaults: maxDepth 4, leaves 1-9 or x

struct RoundTripResult {
  int passed = 0;
  int failed = 0;
  std::string smallestFailureInput;
  std::string smallestFailureReparsed;
  int smallestFailureNodes = -1;
};

RoundTripResult checkRoundTrip(int n) {
  RoundTripResult r;
  Rng rng(kSeed);
  for (int i = 0; i < n; i++) {
    ExprPtr expr = generateRandomExpr(rng, kGenConfig.maxDepth, kGenConfig);
    std::string printed = prettyPrint(expr);

    Lexer lexer(printed);
    auto tokens = lexer.tokenise();
    Parser parser(tokens);
    ExprPtr reparsed = parser.constructTree();

    if (treesEqual(expr, reparsed)) {
      r.passed++;
    } else {
      r.failed++;
      int nodes = countNodes(expr);
      if (r.smallestFailureNodes == -1 || nodes < r.smallestFailureNodes) {
        r.smallestFailureNodes = nodes;
        r.smallestFailureInput = printed;
        r.smallestFailureReparsed = prettyPrint(reparsed);
      }
    }
  }
  return r;
}

struct SimplifierEffectivenessResult {
  std::vector<double> reductions; // fraction of nodes removed, per sample
  int unsupportedCount = 0;
};

SimplifierEffectivenessResult checkSimplifierEffectiveness(int n) {
  SimplifierEffectivenessResult r;
  Rng rng(kSeed); // same seed+config as round-trip => same expression stream
  for (int i = 0; i < n; i++) {
    ExprPtr expr = generateRandomExpr(rng, kGenConfig.maxDepth, kGenConfig);
    DiffResult diff = differentiate(expr, "x");
    if (!diff.result) {
      r.unsupportedCount++;
      continue;
    }
    int rawNodes = countNodes(diff.result);
    ExprPtr simplified = simplify(diff.result);
    int simplifiedNodes = countNodes(simplified);
    double reduction =
        rawNodes > 0 ? 1.0 - (static_cast<double>(simplifiedNodes) / rawNodes)
                     : 0.0;
    r.reductions.push_back(reduction);
  }
  return r;
}

double mean(std::vector<double> v) {
  if (v.empty())
    return 0.0;
  double sum = 0.0;
  for (double x : v)
    sum += x;
  return sum / v.size();
}

double median(std::vector<double> v) {
  if (v.empty())
    return 0.0;
  std::sort(v.begin(), v.end());
  size_t mid = v.size() / 2;
  if (v.size() % 2 == 0)
    return (v[mid - 1] + v[mid]) / 2.0;
  return v[mid];
}

} // namespace

int main() {
  std::printf("formex_stats -- parameters\n");
  std::printf("  seed=%llu  N=%d  maxDepth=%d  leafRange=[%d,%d]  "
              "exponentRange=[%d,%d]\n",
              static_cast<unsigned long long>(kSeed), kSampleCount,
              kGenConfig.maxDepth, kGenConfig.minLeafNumber,
              kGenConfig.maxLeafNumber, kGenConfig.minExponent,
              kGenConfig.maxExponent);
  std::printf("\n");

  RoundTripResult rt = checkRoundTrip(kSampleCount);
  std::printf("Round-trip (parse -> print -> re-parse, structural compare)\n");
  std::printf("  passed: %d / %d (%.2f%%)\n", rt.passed, kSampleCount,
              100.0 * rt.passed / kSampleCount);
  if (rt.failed > 0) {
    std::printf("  FAILED: %d\n", rt.failed);
    std::printf("  smallest failing expression (%d nodes):\n",
                 rt.smallestFailureNodes);
    std::printf("    original : %s\n", rt.smallestFailureInput.c_str());
    std::printf("    reparsed : %s\n", rt.smallestFailureReparsed.c_str());
  }
  std::printf("\n");

  SimplifierEffectivenessResult se =
      checkSimplifierEffectiveness(kSampleCount);
  std::printf("Simplifier effectiveness (raw derivative vs simplified "
              "derivative, node count)\n");
  std::printf("  samples differentiated: %zu  unsupported: %d\n",
               se.reductions.size(), se.unsupportedCount);
  std::printf("  mean reduction:   %.1f%%\n", 100.0 * mean(se.reductions));
  std::printf("  median reduction: %.1f%%\n", 100.0 * median(se.reductions));
  std::printf("\n");

  std::printf("Derivative verification (central difference): PENDING -- "
              "blocked on evaluator placement decision, see item 4.\n");

  return 0;
}
