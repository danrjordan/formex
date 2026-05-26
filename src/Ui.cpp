#include "formex/Ui.h"
#include "formex/Differentiator.h"
#include "formex/Evaluator.h"
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"
#include <cmath>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

using namespace ftxui;

void runUi() {
  auto screen = ScreenInteractive::Fullscreen();

  std::string input;
  auto inputComponent = Input(&input, "enter expression...");

  std::vector<Step> steps;
  std::string result;

  ExprPtr currentTree = nullptr;
  ExprPtr resultTree = nullptr;

  bool showGraph = false;

  struct HistoryEntry {
    std::string input;
    std::string result;
    std::vector<Step> steps;
  };
  std::vector<HistoryEntry> history;
  int selectedHistory = 0;

  auto graphPanel = [&] {
    const int YLABEL_W = 7;
    // fill terminal: bottom chrome = separator+historyBar+separator+input = 4
    // graph chrome  = legend+separator+x-labels = 3
    int COLS = std::max(20, screen.dimx() - YLABEL_W);
    int ROWS = std::max(5, screen.dimy() - 7);

    float xMin = -10.0f, xMax = 10.0f;
    float yMin = 1e9f, yMax = -1e9f;

    auto scanRange = [&](const ExprPtr &tree) {
      if (!tree)
        return;
      for (int c = 0; c < COLS; c++) {
        float t = (float)c / (COLS - 1);
        float x = xMin + t * (xMax - xMin);
        float y = (float)evaluate(tree, x);
        if (std::isfinite(y)) {
          yMin = std::min(yMin, y);
          yMax = std::max(yMax, y);
        }
      }
    };

    scanRange(currentTree);
    scanRange(resultTree);

    float pad = (yMax - yMin) * 0.15f;
    yMin -= pad;
    yMax += pad;
    if (yMax - yMin < 1e-3f) {
      yMin -= 1.0f;
      yMax += 1.0f;
    }

    // round to a nice 1/2/5 × 10^n step given a target tick count
    auto niceStep = [](float range, int target) -> float {
      float raw = range / target;
      float mag = std::pow(10.0f, std::floor(std::log10(raw)));
      float n = raw / mag;
      float nice = n < 1.5f ? 1.0f : n < 3.5f ? 2.0f : n < 7.5f ? 5.0f : 10.0f;
      return nice * mag;
    };

    auto fmtTick = [](float v, float step) -> std::string {
      char buf[16];
      if (step >= 1.0f)
        snprintf(buf, sizeof(buf), "%.0f", v);
      else if (step >= 0.1f)
        snprintf(buf, sizeof(buf), "%.1f", v);
      else
        snprintf(buf, sizeof(buf), "%.2f", v);
      return buf;
    };

    // bits: 1=f(x) green, 2=f'(x) orange, 4=axis
    std::vector<std::vector<uint8_t>> grid(ROWS, std::vector<uint8_t>(COLS, 0));

    int axisRow = (int)((1.0f - (0.0f - yMin) / (yMax - yMin)) * (ROWS - 1));
    int axisCol = (int)((-xMin / (xMax - xMin)) * (COLS - 1));
    axisRow = std::max(0, std::min(ROWS - 1, axisRow));
    axisCol = std::max(0, std::min(COLS - 1, axisCol));
    for (int c = 0; c < COLS; c++)
      grid[axisRow][c] |= 4;
    for (int r = 0; r < ROWS; r++)
      grid[r][axisCol] |= 4;

    auto plotLine = [&](int x0, int y0, int x1, int y1, uint8_t bit) {
      int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
      int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
      int err = dx - dy;
      while (true) {
        if (x0 >= 0 && x0 < COLS && y0 >= 0 && y0 < ROWS)
          grid[y0][x0] |= bit;
        if (x0 == x1 && y0 == y1)
          break;
        int e2 = 2 * err;
        if (e2 > -dy) {
          err -= dy;
          x0 += sx;
        }
        if (e2 < dx) {
          err += dx;
          y0 += sy;
        }
      }
    };

    auto plotCurve = [&](const ExprPtr &tree, uint8_t bit) {
      if (!tree)
        return;
      int prevC = -1, prevR = -1;
      for (int c = 0; c < COLS; c++) {
        float t = (float)c / (COLS - 1);
        float x = xMin + t * (xMax - xMin);
        float y = (float)evaluate(tree, x);
        if (!std::isfinite(y)) {
          prevC = prevR = -1;
          continue;
        }
        int r = (int)((1.0f - (y - yMin) / (yMax - yMin)) * (ROWS - 1));
        if (r >= 0 && r < ROWS) {
          if (prevC >= 0 && std::abs(r - prevR) < ROWS / 2)
            plotLine(prevC, prevR, c, r, bit);
          else
            grid[r][c] |= bit;
          prevC = c;
          prevR = r;
        } else {
          prevC = prevR = -1;
        }
      }
    };

    plotCurve(resultTree, 2);
    plotCurve(currentTree, 1);

    // precompute y tick positions: nice step, labels at round values
    float yStep = niceStep(yMax - yMin, std::max(3, ROWS / 6));
    float yFirst = std::ceil(yMin / yStep) * yStep;
    std::vector<std::pair<int, std::string>> yTicks;
    for (float tv = yFirst; tv <= yMax + yStep * 0.01f; tv += yStep) {
      int tr =
          (int)std::round((1.0f - (tv - yMin) / (yMax - yMin)) * (ROWS - 1));
      if (tr >= 0 && tr < ROWS)
        yTicks.emplace_back(tr, fmtTick(tv, yStep));
    }

    // build row -> label map for O(1) lookup
    std::vector<std::string> yLabel(ROWS);
    for (auto &[tr, lbl] : yTicks) {
      int pad = YLABEL_W - 1 - (int)lbl.size();
      std::string s = (pad > 0 ? std::string(pad, ' ') : "") + lbl + " ";
      s.resize(YLABEL_W, ' ');
      yLabel[tr] = s;
    }

    Elements rowElements;
    for (int r = 0; r < ROWS; r++) {
      Elements cells;
      if (!yLabel[r].empty())
        cells.push_back(text(yLabel[r]) | dim | color(Color::RGB(70, 70, 70)));
      else
        cells.push_back(text(std::string(YLABEL_W, ' ')));

      for (int c = 0; c < COLS; c++) {
        uint8_t cell = grid[r][c];
        if (cell & 1)
          cells.push_back(text("●") | color(Color::RGB(100, 200, 100)));
        else if (cell & 2)
          cells.push_back(text("●") | color(Color::RGB(255, 165, 0)));
        else if (cell & 4)
          cells.push_back(text(r == axisRow && c == axisCol ? "┼"
                               : r == axisRow               ? "─"
                                                            : "│") |
                          color(Color::RGB(60, 60, 60)) | dim);
        else
          cells.push_back(text(" "));
      }
      rowElements.push_back(hbox(cells));
    }

    // x-axis label row: nice step, ~one label per 12 chars
    float xStep = niceStep(xMax - xMin, std::max(3, COLS / 12));
    float xFirst = std::ceil(xMin / xStep) * xStep;
    std::string xAxisStr(YLABEL_W + COLS, ' ');
    for (float tv = xFirst; tv <= xMax + xStep * 0.01f; tv += xStep) {
      float pos = (tv - xMin) / (xMax - xMin) * (COLS - 1);
      int col = (int)std::round(pos);
      std::string s = fmtTick(tv, xStep);
      int start = YLABEL_W + col - (int)s.size() / 2;
      for (int i = 0; i < (int)s.size(); i++)
        if (start + i >= 0 && start + i < (int)xAxisStr.size())
          xAxisStr[start + i] = s[i];
    }
    rowElements.push_back(text(xAxisStr) | dim | color(Color::RGB(70, 70, 70)));

    return vbox({
               hbox({
                   text("GRAPH") | dim | color(Color::Green),
                   text("  ● f(x)") | color(Color::RGB(100, 200, 100)),
                   text("  ● f'(x)") | color(Color::RGB(255, 165, 0)),
                   filler(),
                   text("[" + std::to_string((int)xMin) + ", " +
                        std::to_string((int)xMax) + "]") |
                       dim | color(Color::RGB(60, 60, 60)),
               }),
               separator(),
               vbox(rowElements),
           }) |
           flex;
  };

  auto workingPanel = [&] {
    Elements rows;
    rows.push_back(text("WORKING") | dim | color(Color::Green));
    rows.push_back(separator());
    for (size_t i = 0; i < steps.size(); i++) {
      rows.push_back(hbox({
          text(std::to_string(i + 1) + "  ") | dim | color(Color::Green),
          text(steps[i].rule) | color(Color::Green),
      }));
      if (!steps[i].expr.empty()) {
        rows.push_back(hbox({
            text("    "),
            text(steps[i].expr) | dim | color(Color::RGB(100, 150, 100)),
            text(" -> ") | dim | color(Color::Green),
            text(steps[i].result) | color(Color::RGB(255, 165, 0)),
        }));
      }
    }
    return vbox(rows) | flex;
  };

  auto resultPanel = [&] {
    return vbox({
               text("RESULT") | dim | color(Color::Green),
               separator(),
               text(result) | color(Color::RGB(255, 165, 0)) | bold,
               filler(),
               hbox({
                   text(" symbolic ") | color(Color::Black) |
                       bgcolor(Color::RGB(255, 165, 0)),
                   text("  0.1ms") | dim | color(Color::Green),
               }),
           }) |
           flex;
  };

  auto historyBar = [&] {
    Elements tabs;
    for (size_t i = 0; i < history.size(); i++) {
      auto tab = hbox({
          text(" " + history[i].input + " ") | color(Color::Green),
          text(history[i].result + " ") | color(Color::RGB(255, 165, 0)) | dim,
      });
      if ((int)i == selectedHistory)
        tab = tab | bgcolor(Color::RGB(40, 40, 40));
      tabs.push_back(tab);
      if (i < history.size() - 1)
        tabs.push_back(text("│") | dim | color(Color::Green));
    }
    return hbox(tabs);
  };

  auto root = Renderer(inputComponent, [&] {
    return vbox({
               (showGraph
                    ? (graphPanel() | flex)
                    : (hbox({
                           workingPanel() | flex,
                           separator(),
                           resultPanel() | size(ftxui::WIDTH, ftxui::EQUAL, 30),
                       }) |
                       flex)),
               separator(),
               historyBar(),
               separator(),
               hbox({
                   text(" SYM ") | color(Color::Black) | bgcolor(Color::Yellow),
                   text(" "),
                   inputComponent->Render() | flex,
                   text(" Tab history  ^G graph  ^F factor  ^S steps ") |
                       color(Color::Green) | dim,
               }),
           }) |
           bgcolor(Color::RGB(18, 18, 18));
  });

  auto app = CatchEvent(root, [&](Event event) {
    if (event == Event::ArrowLeft && selectedHistory > 0) {
      selectedHistory--;
      steps = history[selectedHistory].steps;
      result = history[selectedHistory].result;
      return true;
    }
    if (event == Event::ArrowRight &&
        selectedHistory < (int)history.size() - 1) {
      selectedHistory++;
      steps = history[selectedHistory].steps;
      result = history[selectedHistory].result;
      return true;
    }
    if (event == Event::CtrlG) {
      showGraph = !showGraph;
      return true;
    }
    if (event == Event::Return) {
      try {
        if (input.empty())
          return true;

        Lexer lexer(input);
        auto tokens = lexer.tokenise();
        Parser parser(tokens);
        auto tree = parser.constructTree();
        currentTree = clone(tree);
        auto [diffResult, diffSteps] = differentiate(tree, "x");

        if (!diffResult) {
          result = "error: unsupported expression";
          steps = {Step{"unsupported", input, ""}};
          input.clear();
          return true;
        }

        auto simplified = simplify(diffResult);
        resultTree = clone(simplified);
        steps = diffSteps;
        steps.insert(steps.begin(), Step{"input: " + input, "", ""});
        result = prettyPrint(simplified);
        history.push_back({input, result, steps});
        selectedHistory = history.size() - 1;
        input.clear();
      } catch (const std::exception &e) {
        result = std::string("error: ") + e.what();
        steps = {Step{"parse error", input, ""}};
      }
      return true;
    }
    return false;
  });

  screen.Loop(app);
}