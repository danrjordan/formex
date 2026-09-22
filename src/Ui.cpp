#include "formex/Ui.h"
#include "formex/Differentiator.h"
#include "formex/Lexer.h"
#include "formex/LinearAlgebra.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

using namespace ftxui;

enum class Mode { Differentiator, LinearAlgebra };

void runUi() {
  auto screen = ScreenInteractive::Fullscreen();

  std::string input;
  auto inputComponent = Input(&input, "enter expression...");

  Mode mode = Mode::Differentiator;
  std::vector<Step> steps;
  std::string result;

  struct HistoryEntry {
    std::string input;
    std::string result;
    std::vector<Step> steps;
    Mode mode;
  };
  std::vector<HistoryEntry> history;
  int selectedHistory = 0;

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
    std::string modeLabel =
        mode == Mode::Differentiator ? " symbolic " : " linear algebra ";
    return vbox({
               text("RESULT") | dim | color(Color::Green),
               separator(),
               text(result) | color(Color::RGB(255, 165, 0)) | bold,
               filler(),
               hbox({
                   text(modeLabel) | color(Color::Black) |
                       bgcolor(Color::RGB(255, 165, 0)),
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
    bool isDiff = mode == Mode::Differentiator;
    return vbox({
               hbox({
                   workingPanel() | flex,
                   separator(),
                   resultPanel() | size(ftxui::WIDTH, ftxui::EQUAL, 30),
               }) | flex,
               separator(),
               historyBar(),
               separator(),
               hbox({
                   text(isDiff ? " SYM " : " MAT ") | color(Color::Black) |
                       bgcolor(isDiff ? Color::Yellow : Color::Cyan),
                   text(" "),
                   inputComponent->Render() | flex,
                   text(" ← → history  Tab mode ") | color(Color::Green) |
                       dim,
               }),
           }) |
           bgcolor(Color::RGB(18, 18, 18));
  });

  auto app = CatchEvent(root, [&](Event event) {
    if (event == Event::Tab) {
      mode = mode == Mode::Differentiator ? Mode::LinearAlgebra
                                          : Mode::Differentiator;
      return true;
    }
    if (event == Event::ArrowLeft && selectedHistory > 0) {
      selectedHistory--;
      steps = history[selectedHistory].steps;
      result = history[selectedHistory].result;
      mode = history[selectedHistory].mode;
      return true;
    }
    if (event == Event::ArrowRight &&
        selectedHistory < (int)history.size() - 1) {
      selectedHistory++;
      steps = history[selectedHistory].steps;
      result = history[selectedHistory].result;
      mode = history[selectedHistory].mode;
      return true;
    }
    if (event == Event::Return) {
      if (input.empty())
        return true;

      if (mode == Mode::LinearAlgebra) {
        auto solveResult = solveLinear(input);
        result = solveResult.result;
        steps.clear();
        steps.push_back(Step{"input: " + input, "", ""});
        for (const auto &s : solveResult.steps)
          steps.push_back(Step{s.rule, s.expr, s.result});
        history.push_back({input, result, steps, mode});
        selectedHistory = history.size() - 1;
        input.clear();
        return true;
      }

      try {
        Lexer lexer(input);
        auto tokens = lexer.tokenise();
        Parser parser(tokens);
        auto tree = parser.constructTree();
        tree = simplify(tree); // fold constant subexpressions, e.g. x^(2+1)
        auto [diffResult, diffSteps] = differentiate(tree, "x");

        if (!diffResult) {
          result = "error: unsupported expression";
          steps = {Step{"unsupported", input, ""}};
          input.clear();
          return true;
        }

        auto simplified = simplify(diffResult);
        steps = diffSteps;
        steps.insert(steps.begin(), Step{"input: " + input, "", ""});
        result = prettyPrint(simplified);
        history.push_back({input, result, steps, mode});
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