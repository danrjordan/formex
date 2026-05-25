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

  auto root = Renderer(inputComponent, [&] {
    return vbox({
               hbox({
                   workingPanel(),
                   separator(),
                   resultPanel(),
               }) | flex,
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
    if (event == Event::Return) {
      Lexer lexer(input);
      auto tokens = lexer.tokenise();
      Parser parser(tokens);
      auto tree = parser.constructTree();
      auto [diffResult, diffSteps] = differentiate(tree, "x");
      auto simplified = simplify(diffResult);

      steps = diffSteps;
      steps.insert(steps.begin(), Step{"input: " + input, ""});
      result = prettyPrint(simplified);
      input.clear();
      return true;
    }
    return false;
  });

  screen.Loop(app);
}