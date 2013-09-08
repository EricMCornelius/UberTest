#pragma once

#include <iostream>
#include <chrono>

#include <unordered_map>
#include <string>
#include <sstream>

#include <stdio.h>
#include <unistd.h>

namespace ut {

struct OstreamReporter {
  OstreamReporter(std::ostream& out_)
    : out(out_)
  {

  }

  std::ostream& out;
  std::size_t indentation = 0;
  std::size_t indentation_size = 2;

  std::unordered_map<std::string, std::size_t> color_map = {
    {"black", 0},
    {"red", 1},
    {"green", 2},
    {"yellow", 3},
    {"blue", 4},
    {"magenta", 5},
    {"cyan", 6},
    {"white", 7}
  };

  std::unordered_map<std::string, std::size_t> attributes = {
    {"foreground", 30},
    {"background", 40},
    {"foreground bright", 90},
    {"background bright", 100}
  };

  std::string pad() {
    return "\n" + std::string(indentation, ' ');
  }

  void increaseIndentation() {
    indentation += indentation_size;
  }

  void decreaseIndentation() {
    indentation -= indentation_size;
  }

  void startColor(const std::string& name, const std::string& attribute = "foreground") {
    out << "\u001b[" << color_map[name] + attributes[attribute] << "m";
  }

  void endColor() {
    out << "\u001b[0m";
  }

  enum class Color : std::uint8_t {
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
  };

  // http://stackoverflow.com/questions/1312922/detect-if-stdin-is-a-terminal-or-pipe-in-c-c-qt
  bool isTerminal() {
    return (isatty(fileno(stdout)));
  }

  void print_impl() {
    std::cout << std::endl;
  }

  template <typename Head, typename... Args>
  void print_impl(const Head& msg, const Args&... args) {
    out << msg << " ";
    print_impl(args...);
  }

  template <typename... Args>
  void print_impl(const Color& color, const Args&... args) {
    if (isTerminal()) {
      endColor();
      switch(color) {
        case Color::Black:
          startColor("black");
          break;
        case Color::Red:
          startColor("red");
          break;
        case Color::Green:
          startColor("green");
          break;
        case Color::Yellow:
          startColor("yellow");
          break;
        case Color::Blue:
          startColor("blue");
          break;
        case Color::Magenta:
          startColor("magenta");
          break;
        case Color::Cyan:
          startColor("cyan");
          break;
        case Color::White:
          startColor("white");
          break;
      }
    }
    print_impl(args...);
  }

  template <typename... Args>
  void print(const Args&... args) {
    if (isTerminal())
      startColor("white");
    print_impl(args...);
    if (isTerminal())
      endColor();
  }

  // http://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string
  struct redirect {
    redirect(std::ostream& buffer)
      : _buffer(buffer), _old(buffer.rdbuf(_str.rdbuf()))
    { }

    std::string contents() {
      return _str.str();
    }

    ~redirect( ) {
      _buffer.rdbuf(_old);
    }

  private:
    std::stringstream _str;
    std::ostream& _buffer;
    std::streambuf* _old;
  };

  std::vector<std::shared_ptr<redirect>> redirections;

  virtual void testStarted(const Test& t) {
    print(pad(), Color::Yellow, "Test started:", Color::Cyan, t.name);
    redirections.emplace_back(std::make_shared<redirect>(std::cerr));
    redirections.emplace_back(std::make_shared<redirect>(std::cout));
  }

  virtual void testFailed(const Test& t) {
    auto stdout = redirections.back()->contents();
    redirections.pop_back();
    auto stderr = redirections.back()->contents();
    redirections.pop_back();
    bool us = (t.seconds < 0.001);
    print(pad(), Color::Red, "Failure:",
          Color::Yellow, pad(), "execution time:", Color::White, (us) ? t.microseconds : t.seconds, (us) ? "(us)" : "(s)",
          Color::Yellow, pad(), "stdout:", Color::White, stdout,
          Color::Yellow, pad(), "stderr:", Color::White, stderr,
          Color::Yellow, pad(), "failure:", Color::White, t.message);
  }

  virtual void testSucceeded(const Test& t) {
    auto stdout = redirections.back()->contents();
    redirections.pop_back();
    auto stderr = redirections.back()->contents();
    redirections.pop_back();
    bool us = (t.seconds < 0.001);
    print(pad(), Color::Green, "Success:",
          Color::Yellow, pad(), "execution time:", Color::White, (us) ? t.microseconds : t.seconds, (us) ? "(us)" : "(s)",
          Color::Yellow, pad(), "stdout:", Color::White, stdout,
          Color::Yellow, pad(), "stderr:", Color::White, stderr);
  }

  virtual void suiteStarted(const Suite& s) {
    print(pad(), Color::Yellow, "Suite Started:", Color::Cyan, s.name);
    increaseIndentation();
  }

  virtual void suiteFailed(const Suite& s) {
    decreaseIndentation();
    print(pad(), Color::Red, "Suite Failed:", Color::Cyan, s.name);
  }

  virtual void suiteSucceeded(const Suite& s) {
    decreaseIndentation();
    print(pad(), Color::Green, "Suite Succeeded:", Color::Cyan, s.name);
  }
};

}
