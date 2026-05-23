#include "formex/Ui.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

void runUi() {
  auto screen = ScreenInteractive::Fullscreen();

  std::string input;
  auto inputComponent = Input(&input, "enter expression...");

  std::vector<std::string> steps = {
      "input: x^2 - 5x + 6",
      "apply power rule",
      "simplify",
  };
  std::string result = "2*x";

  auto workingPanel = [&] {
    Elements rows;
    rows.push_back(text("WORKING") | dim | color(Color::Green));
    rows.push_back(separator());
    for (size_t i = 0; i < steps.size(); i++) {
      rows.push_back(hbox({
          text(std::to_string(i + 1) + "  ") | dim | color(Color::Green),
          text(steps[i]) | color(Color::Green),
      }));
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

  screen.Loop(root);
}