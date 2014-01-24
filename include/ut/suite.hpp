#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <stdexcept>
#include <unordered_map>

#include <sstream>

#include <ut/test.hpp>
#include <ut/reporter.hpp>

namespace ut {

typedef std::function<void(parent_name_getter, ActionAccumulator&, ActionAccumulator&, ActionAccumulator&, ActionAccumulator&, TestAccumulator&)> suite_initializer;

struct Suite : std::enable_shared_from_this<Suite> {
  std::vector<Action> _before;
  std::vector<Action> _beforeEach;
  std::vector<Action> _after;
  std::vector<Action> _afterEach;

  std::vector<Test> tests;
  std::vector<std::shared_ptr<Suite>> suites;

  std::shared_ptr<Suite> parent = nullptr;
  std::string name;
  std::string path;
  suite_initializer initializer = nullptr;
  mutable std::size_t failures = 0;
  mutable std::size_t successes = 0;

  Suite() {}

  Suite(const char* name_)
    : name(name_), path(name_) {}

  Suite(const std::shared_ptr<Suite> parent_, const std::string& name_, const std::string& path_, const suite_initializer& initializer_)
    : parent(parent_), name(name_), path(path_), initializer(initializer_)
  {

  }

  void initialize() {
    parent->suites.push_back(shared_from_this());
    initialize(initializer);
  }

  void initialize(const suite_initializer& initializer_) {
    ActionAccumulator before(_before);
    ActionAccumulator beforeEach(_beforeEach);
    ActionAccumulator after(_after);
    ActionAccumulator afterEach(_afterEach);
    TestAccumulator it(tests);

    auto parent_getter = [&]() {
      return path;
    };

    initializer_(parent_getter, before, beforeEach, after, afterEach, it);
  }

  void execute() const {
    Reporter defaultReporter;
    execute(defaultReporter);
  }

  template <typename Cont>
  void call(const Cont& c) const {
    for (const auto& e : c)
      e.run();
  }

  template <typename Reporter = Reporter>
  void execute(Reporter& reporter) const {
    failures = 0;
    successes = 0;

    reporter.suiteStarted(*this);

    call(_before);

    for (const auto& test : tests) {
      call(_beforeEach);

      reporter.testStarted(test);
      test.run();
      if (test.failed) {
        reporter.testFailed(test);
        ++failures;
      }
      else {
        reporter.testSucceeded(test);
        ++successes;
      }

      call(_afterEach);
    }

    call(_after);

    for (const auto& s : suites) {
      s->execute(reporter);
      successes += s->successes;
      failures += s->failures;
    }

    if (failures > 0)
      reporter.suiteFailed(*this);
    else
      reporter.suiteSucceeded(*this);
  }
};

}
