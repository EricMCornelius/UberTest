#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <stdexcept>
#include <unordered_map>

#include <timer.hpp>

#include <iostream>

namespace ut {

struct Suite;

typedef std::function<void()> callback;
typedef std::function<void(const callback&)> registration;
typedef std::function<void(bool)> bool_callback;
typedef std::function<void(const bool_callback&)> async_callback;
typedef std::function<void(const char*, const callback&)> tagged_registration;
typedef std::function<std::string()> parent_name_getter;

struct CallbackAccumulator {
  CallbackAccumulator(std::vector<callback>& callbacks, std::vector<async_callback>& async_callbacks)
    : _callbacks(callbacks), _async_callbacks(async_callbacks) {}

  void operator()(const callback& cb) {
    _callbacks.emplace_back(cb);
  }

  void operator()(const async_callback& cb) {
    _async_callbacks.emplace_back(cb);
  }

  std::vector<callback>& _callbacks;
  std::vector<async_callback>& _async_callbacks;
};

struct Test {
  const std::string name;
  const callback cb;
  mutable std::string message;
  mutable bool failed = false;
  mutable double seconds = 0;
  mutable std::size_t microseconds = 0;

  void run() const {
    timer t;
    t.start();
    try {
      cb();
    }
    catch(std::exception& e) {
      failed = true;
      message = e.what();
    }
    t.stop();
    seconds = t.seconds();
    microseconds = t.count();
  }

  Test(const std::string& name_, const callback& cb_)
    : name(name_), cb(cb_) {}
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

typedef std::function<void(parent_name_getter, CallbackAccumulator&, CallbackAccumulator&, CallbackAccumulator&, CallbackAccumulator&, tagged_registration)> suite_initializer;

struct Suite : std::enable_shared_from_this<Suite> {
  std::vector<callback> _before;
  std::vector<async_callback> _beforeAsync;
  std::vector<callback> _beforeEach;
  std::vector<async_callback> _beforeEachAsync;
  std::vector<callback> _after;
  std::vector<async_callback> _afterAsync;
  std::vector<callback> _afterEach;
  std::vector<async_callback> _afterEachAsync;

  std::vector<Test> tests;
  std::vector<std::shared_ptr<Suite>> suites;

  std::shared_ptr<Suite> parent = nullptr;
  std::string name;
  std::string path;
  suite_initializer initializer = nullptr;
  mutable bool failed = false;

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
    CallbackAccumulator before(_before, _beforeAsync);
    CallbackAccumulator beforeEach(_beforeEach, _beforeEachAsync);
    CallbackAccumulator after(_after, _afterAsync);
    CallbackAccumulator afterEach(_afterEach, _afterEachAsync);

    auto it = [&](const char* tag, const callback& cb) {
      tests.emplace_back(tag, cb);
    };

    auto parent_getter = [&]() {
      return path;
    };

    initializer_(parent_getter, before, beforeEach, after, afterEach, it);
  }

  void call(const std::vector<callback>& cbs) const {
    for (const auto& cb : cbs)
      cb();
  }

  void call(const std::vector<async_callback>& cbs) const {
    for (const auto& cb : cbs) {
      auto promise = std::promise<bool>();
      std::thread t([&]() {
        cb([&](bool value) {
          promise.set_value(value);
        });
      });
      auto future = promise.get_future();
      future.get();
      t.join();
    }
  }

  void execute() const {
    Reporter defaultReporter;
    execute(defaultReporter);
  }

  template <typename Reporter = Reporter>
  void execute(Reporter& reporter) const {
    failed = false;
    reporter.suiteStarted(*this);

    call(_before);
    call(_beforeAsync);

    for (const auto& test : tests) {
      call(_beforeEach);
      call(_beforeEachAsync);

      reporter.testStarted(test);
      test.run();
      if (test.failed) {
        reporter.testFailed(test);
        failed = true;
      }
      else {
        reporter.testSucceeded(test);
      }

      call(_afterEach);
      call(_afterAsync);
    }

    call(_after);
    call(_afterAsync);

    for (const auto& s : suites) {
      s->execute(reporter);
      if (s->failed)
        failed = true;
    }

    if (failed)
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
  [] (parent_name_getter parent, CallbackAccumulator& before, CallbackAccumulator& beforeEach, CallbackAccumulator& after, CallbackAccumulator& afterEach, tagged_registration it) { \

#define done(tag) \
}); \
