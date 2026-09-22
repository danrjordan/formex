#include "Evaluator.h"
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
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr uint64_t kSeed = 1337;
constexpr uint64_t kXSampleSeed = 1338; // separate stream: sample-count changes
                                        // here must not perturb the expression
                                        // sequence used by the other checks
constexpr int kSampleCount = 500;
constexpr ExprGenConfig kGenConfig{}; // defaults: maxDepth 4, leaves 1-9 or x

constexpr int kPointsPerExpr = 5;
constexpr double kXRangeMin = -5.0;
constexpr double kXRangeMax = 5.0;
constexpr double kH = 1e-5;      // central-difference step
constexpr double kAbsTol = 1e-4; // combined tolerance: |sym-num| <= AbsTol +
constexpr double kRelTol = 1e-4; //   RelTol * max(|sym|, |num|)

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

struct DerivativeCheckResult {
  int passed = 0;
  int failed = 0;
  int skipped = 0;
  int smallestFailureNodes = -1;
  std::string smallestFailureExprStr;
  double smallestFailureX = 0;
  double smallestFailureSymbolic = 0;
  double smallestFailureNumeric = 0;
};

// Compares the symbolic derivative against a central-difference numeric
// estimate at kPointsPerExpr random points per expression. Points where any
// evaluation is undefined (division by zero, non-finite pow, etc.) are
// skipped and counted separately rather than treated as pass or fail.
DerivativeCheckResult checkDerivative(int n) {
  DerivativeCheckResult r;
  Rng exprRng(kSeed); // same seed+config as the other checks => same exprs
  Rng xRng(kXSampleSeed);

  for (int i = 0; i < n; i++) {
    ExprPtr expr = generateRandomExpr(exprRng, kGenConfig.maxDepth, kGenConfig);
    DiffResult diff = differentiate(expr, "x");
    if (!diff.result)
      continue; // tallied as "unsupported" by the simplifier-effectiveness pass
    ExprPtr derivative = simplify(diff.result);

    for (int p = 0; p < kPointsPerExpr; p++) {
      double x = kXRangeMin + xRng.nextDouble() * (kXRangeMax - kXRangeMin);

      auto fPlus = evaluate(expr, "x", x + kH);
      auto fMinus = evaluate(expr, "x", x - kH);
      auto symbolic = evaluate(derivative, "x", x);
      if (!fPlus || !fMinus || !symbolic) {
        r.skipped++;
        continue;
      }

      double numeric = (*fPlus - *fMinus) / (2.0 * kH);
      if (!std::isfinite(numeric)) {
        r.skipped++;
        continue;
      }

      double gap = std::fabs(*symbolic - numeric);
      double tol =
          kAbsTol + kRelTol * std::max(std::fabs(*symbolic), std::fabs(numeric));

      if (gap <= tol) {
        r.passed++;
      } else {
        r.failed++;
        int nodes = countNodes(expr);
        if (r.smallestFailureNodes == -1 || nodes < r.smallestFailureNodes) {
          r.smallestFailureNodes = nodes;
          r.smallestFailureExprStr = prettyPrint(expr);
          r.smallestFailureX = x;
          r.smallestFailureSymbolic = *symbolic;
          r.smallestFailureNumeric = numeric;
        }
      }
    }
  }
  return r;
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
  std::printf("  derivative check: xSampleSeed=%llu  pointsPerExpr=%d  "
              "xRange=[%.1f,%.1f]  h=%g  absTol=%g  relTol=%g\n",
              static_cast<unsigned long long>(kXSampleSeed), kPointsPerExpr,
              kXRangeMin, kXRangeMax, kH, kAbsTol, kRelTol);
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

  DerivativeCheckResult dv = checkDerivative(kSampleCount);
  int dvTotal = dv.passed + dv.failed + dv.skipped;
  std::printf("Derivative verification (symbolic vs central difference)\n");
  std::printf("  points evaluated: %d  (over %d expressions x up to %d points each)\n",
               dvTotal, kSampleCount, kPointsPerExpr);
  std::printf("  passed:  %d (%.2f%%)\n", dv.passed,
              100.0 * dv.passed / dvTotal);
  std::printf("  failed:  %d (%.2f%%)\n", dv.failed,
              100.0 * dv.failed / dvTotal);
  std::printf("  skipped: %d (%.2f%%)\n", dv.skipped,
              100.0 * dv.skipped / dvTotal);
  if (dv.failed > 0) {
    std::printf("  smallest failing expression (%d nodes):\n",
                 dv.smallestFailureNodes);
    std::printf("    f(x)      = %s\n", dv.smallestFailureExprStr.c_str());
    std::printf("    x         = %.6f\n", dv.smallestFailureX);
    std::printf("    symbolic  = %.6f\n", dv.smallestFailureSymbolic);
    std::printf("    numeric   = %.6f\n", dv.smallestFailureNumeric);
  }

  return 0;
}
