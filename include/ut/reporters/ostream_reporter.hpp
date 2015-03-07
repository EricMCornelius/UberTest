#pragma once

#include <iostream>
#include <chrono>

#include <unordered_map>
#include <string>
#include <sstream>

#include <stdio.h>
#include <unistd.h>

#include <ut/reporter.hpp>
#include <ut/test.hpp>

namespace ut {

struct OstreamReporter : Reporter {
  OstreamReporter(std::ostream& out_)
    : out(out_)
  {
    execution_time_str = (utf8 ? "\u231B" : "execution time:");
    success_str = (utf8 ? "\u2713" : "succeeded");
    successes_str = (utf8 ? "\u2713" : "successes:");
    failure_str = (utf8 ? "\u2717" : "failed");
    failures_str = (utf8 ? "\u2717" : "failures:");
    stub_str = (utf8 ? "\u2126" : "stubbed");
    stubs_str = (utf8 ? "\u2126" : "stubs:");

    newline_after_test = !compact;
    newline_after_suite_start = !compact;
    newline_after_suite_end = !compact;
    message_str = (compact ? "" : "message:");
    location_str = (compact ? "" : "location:");

    print_stdout = verbose;
    print_stderr = verbose;
    print_stack = verbose;
  }

  std::ostream& out;
  std::size_t indentation = 0;
  std::size_t indentation_size = 2;

  bool print_execution_time = true;
  bool print_stack = false;
  bool print_location = true;
  bool print_stdout = false;
  bool print_stderr = false;
  bool newline_after_suite_start = false;
  bool newline_after_suite_end = false;
  bool newline_after_test = false;

  bool compact = true;
  bool verbose = false;
  bool utf8 = true;

  std::string execution_time_str;
  std::string success_str;
  std::string successes_str;
  std::string failure_str;
  std::string failures_str;
  std::string stub_str;
  std::string stubs_str;
  std::string message_str;
  std::string location_str;

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

  struct padding {
    int size = -1;

    padding() {}
    padding(std::size_t size_) : size(size_) {}
  };

  padding pad() {
    return indentation;
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
    None,
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

  }

  template <typename Head, typename... Args>
  void print_impl(const Head& msg, const Args&... args) {
    out << msg << " ";
    print_impl(args...);
  }

  template <typename... Args>
  void print_impl(const padding& padding, const Args&... args) {
    if (padding.size < 0) {
      out << ' ';
      print_impl(args...);
    }
    else {
      out << '\n' + std::string(padding.size, ' ');
      print_impl(args...);
    }
  }

  template <typename... Args>
  void print_impl(const Color& color, const Args&... args) {
    if (isTerminal()) {
      endColor();
      switch(color) {
        case Color::None:
          break;
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

  virtual void testStubbed(const Test& t) {
    print(pad(), Color::Yellow, Color::Cyan, t.name + ':');
    print((compact ? padding() : pad()), Color::Yellow, stub_str);
    if (newline_after_test)
      print('\n');
  }

  virtual void testStarted(const Test& t) {
    print(pad(), Color::Yellow, Color::Cyan, t.name + ':');
    redirections.emplace_back(std::make_shared<redirect>(std::cerr));
    redirections.emplace_back(std::make_shared<redirect>(std::cout));
    increaseIndentation();
  }

  virtual void testFailed(const Test& t) {
    auto stdout = redirections.back()->contents();
    redirections.pop_back();
    auto stderr = redirections.back()->contents();
    redirections.pop_back();
    bool us = (t.seconds < 0.001);
    print((compact ? padding() : pad()), Color::Red, failure_str);
    increaseIndentation();
    auto padding = compact ? -1 : pad();
    if (t.exception) {
      print(Color::Yellow, padding, message_str, Color::Red, t.exception->what());
      if (print_location && !t.exception->location.empty())
        print(Color::Yellow, padding, location_str, Color::Red, t.exception->location);
    }
    else {
      print(Color::Yellow, padding, message_str, Color::White, t.message);
    }
    if (print_execution_time)
      print(Color::Yellow, padding, execution_time_str, Color::White, (us) ? t.microseconds : t.seconds, (us) ? "(us)" : "(s)");
    if (print_stdout && !stdout.empty())
      print(Color::Yellow, padding, "stdout:", Color::White, stdout);
    if (print_stderr && !stderr.empty())
      print(Color::Yellow, padding, "stderr:", Color::White, stderr);
    if (print_stack && t.exception)
      print(Color::Yellow, padding, "stack:\n", Color::None, t.exception->stack);

    decreaseIndentation();
    decreaseIndentation();
    if (newline_after_test)
      print('\n');
  }

  virtual void testSucceeded(const Test& t) {
    auto stdout = redirections.back()->contents();
    redirections.pop_back();
    auto stderr = redirections.back()->contents();
    redirections.pop_back();
    bool us = (t.seconds < 0.001);
    print((compact ? padding() : pad()), Color::Green, success_str);
    increaseIndentation();
    auto padding = compact ? -1 : pad();
    if (print_execution_time)
      print(Color::Yellow, padding, execution_time_str, Color::White, (us) ? t.microseconds : t.seconds, (us) ? "(us)" : "(s)");
    if (print_stdout && !stdout.empty())
      print(Color::Yellow, padding, "stdout:", Color::White, stdout);
    if (print_stderr && !stderr.empty())
      print(Color::Yellow, padding, "stderr:", Color::White, stderr);
    decreaseIndentation();
    decreaseIndentation();
    if (newline_after_test)
      print('\n');
  }

  virtual void suiteStarted(const Suite& s) {
    if(s.name == "root") {
      print(Color::Yellow, Color::Blue, s.name + ':');
    }
    else {
      print(pad(), Color::Yellow, Color::Blue, s.name + ':');
    }
    if (newline_after_suite_start) {
      print('\n');
    }
    increaseIndentation();
  }

  virtual void suiteFailed(const Suite& s) {
    decreaseIndentation();
    print(pad(), Color::Blue, s.name + " results:");
    increaseIndentation();
    auto padding = compact ? -1 : pad();
    print(Color::Yellow, padding, failures_str, Color::Red, s.failures);
    if (s.successes)
      print(Color::Yellow, padding, successes_str, Color::Green, s.successes);
    if (s.stubs)
      print(Color::Yellow, padding, stubs_str, Color::Blue, s.stubs);
    if (print_execution_time) {
      bool us = (s.microseconds < 1000);
      print(Color::Yellow, padding, execution_time_str, Color::White, (us) ? s.microseconds : s.microseconds / 1000000.0, (us) ? "(us)" : "(s)");
    }
    decreaseIndentation();
    if (newline_after_suite_end && s.name != "root")
      print('\n');
  }

  virtual void suiteSucceeded(const Suite& s) {
    decreaseIndentation();
    print(pad(), Color::Blue, s.name + " results:");
    increaseIndentation();
    auto padding = compact ? -1 : pad();
    print(Color::Yellow, padding, successes_str, Color::Green, s.successes);
    if (s.stubs)
      print(Color::Yellow, padding, stubs_str, Color::Blue, s.stubs);
    if (print_execution_time) {
      bool us = (s.microseconds < 1000);
      print(Color::Yellow, padding, execution_time_str, Color::White, (us) ? s.microseconds : s.microseconds / 1000000.0, (us) ? "(us)" : "(s)");
    }
    decreaseIndentation();
    if (newline_after_suite_end && s.name != "root")
      print('\n');
  }
};

}
