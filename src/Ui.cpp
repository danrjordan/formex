#include "formex/Ui.h"
#include "formex/Differentiator.h"
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"
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

  struct HistoryEntry {
    std::string input;
    std::string result;
    std::vector<Step> steps;
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
               hbox({
                   workingPanel() | flex,
                   separator(),
                   resultPanel() | size(ftxui::WIDTH, ftxui::EQUAL, 30),
               }) |
                   flex,
               separator(),
               historyBar(),
               separator(),
               hbox({
                   text(" SYM ") | color(Color::Black) | bgcolor(Color::Yellow),
                   text(" "),
                   inputComponent->Render() | flex,
                   text(" Tab history  ^F factor  ^S steps ") |
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
    if (event == Event::Return) {
      try {
        if (input.empty())
          return true;

        Lexer lexer(input);
        auto tokens = lexer.tokenise();
        Parser parser(tokens);
        auto tree = parser.constructTree();
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