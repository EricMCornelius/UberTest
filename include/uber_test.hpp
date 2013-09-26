#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <stdexcept>
#include <unordered_map>

#include <timer.hpp>
#include <sstream>

namespace ut {

struct Suite;

typedef std::function<void()> void_callback;
typedef std::function<void(const void_callback&)> registration;

struct callback {
  callback(std::promise<std::string>& p)
    : _p(p) {}

  template <typename T>
  void operator()(const T& obj) const {
    std::stringstream str;
    str << obj;
    _p.set_value(str.str());
  }

  void operator()(const std::string& msg) const {
    _p.set_value(msg);
  }
  void operator()() const {
    _p.set_value("");
  }

  std::promise<std::string>& _p;
};

typedef std::function<void(const callback&)> async_callback;
typedef std::function<std::string()> parent_name_getter;

struct Action {
  const void_callback cb = nullptr;
  const async_callback async_cb = nullptr;
  const bool async = false;

  void run() const {
    if (async)
      run_async();
    else
      run_sync();
  }

  void run_sync() const {
    cb();
  }

  void run_async() const {
    auto promise = std::promise<std::string>();
    std::thread thr([&]() {
      async_cb(callback(promise));
    });
    auto future = promise.get_future();
    auto ret = future.get();
    thr.join();
    if (!ret.empty())
      throw std::runtime_error(ret);
  }

  Action(const void_callback& cb_)
    : cb(cb_) {}

  Action(const async_callback& async_cb_)
    : async_cb(async_cb_), async(true) {}
};

struct ActionAccumulator {
  ActionAccumulator(std::vector<Action>& actions)
    : _actions(actions) {}

  template <typename Cb>
  void operator()(const Cb& cb) {
    _actions.emplace_back(cb);
  }

  std::vector<Action>& _actions;
};

struct Test : public Action {
  const std::string name;
  mutable std::string message;
  mutable bool failed = false;
  mutable double seconds = 0;
  mutable std::size_t microseconds = 0;

  void run() const {
    timer t;
    t.start();
    try {
      Action::run();
    }
    catch(std::exception& e) {
      failed = true;
      message = e.what();
    }
    t.stop();
    seconds = t.seconds();
    microseconds = t.count();
  }

  Test(const std::string& name_, const void_callback& cb_)
    : Action(cb_), name(name_) {}

  Test(const std::string& name_, const async_callback& async_cb_)
    : Action(async_cb_), name(name_) {}
};

struct TestAccumulator {
  TestAccumulator(std::vector<Test>& tests)
    : _tests(tests) {}

  template <typename Cb>
  void operator()(const std::string& name, const Cb& cb) {
    _tests.emplace_back(name, cb);
  }

  std::vector<Test>& _tests;
};

struct Suite;

struct Reporter {
  virtual void testStarted(const Test& t) {}
  virtual void testFailed(const Test& t) {}
  virtual void testSucceeded(const Test& t) {}
  virtual void suiteStarted(const Suite& s) {}
  virtual void suiteFailed(const Suite& s) {}
  virtual void suiteSucceeded(const Suite& s) {}
};

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

struct Registry {
  static std::unordered_map<std::string, std::shared_ptr<Suite>>& registered() {
    static std::unordered_map<std::string, std::shared_ptr<Suite>> _impl;
    return _impl;
  };

  static bool add(const std::string parent_name, const std::string name, const suite_initializer cb) {
    if (parent_name == parent()) {
      if(registered().find("root") == registered().end())
        registered()["root"] = std::make_shared<Suite>("root");
    }

    std::string full = parent_name + "/" + name;
    auto parent = registered()[parent_name];
    auto current = registered().find(full);

    if (current == registered().end()) {
      auto var = std::make_shared<Suite>(parent, name, full, cb);
      registered()[full] = var;
      var->initialize();
    }
    else {
      current->second->initialize(cb);
    }
    return true;
  }

  static std::shared_ptr<Suite> get(const std::string& name) {
    auto current = registered().find(name);
    if (current == registered().end())
      return nullptr;
    return current->second;
  }

  static std::string parent() {
    return "root";
  }
};

inline std::string parent() {
  return Registry::parent();
}

}

#define describe(tag) \
auto tag = Registry::add(parent(), #tag, \
  [] (parent_name_getter parent, ActionAccumulator& before, ActionAccumulator& beforeEach, ActionAccumulator& after, ActionAccumulator& afterEach, TestAccumulator& it) { \

#define done(tag) \
});
