#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <stdexcept>
#include <unordered_map>

#include <ut/timer.hpp>
#include <ut/assertions.hpp>

#include <sstream>

namespace ut {

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
    if (!cb && !async_cb) {
      // stubbed
      return;
    }

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
    ut_assert(ret.empty(), ret);
  }

  Action() {}

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
  bool is_stub = false;
  mutable std::shared_ptr<ut::Exception> exception = nullptr;
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
    catch(ut::Exception& e) {
      failed = true;
      exception = std::make_shared<ut::Exception>(std::move(e));
    }
    catch(std::exception& e) {
      failed = true;
      message = e.what();
    }
    t.stop();
    seconds = t.seconds();
    microseconds = t.count();
  }

  Test(const std::string& name_)
    : Action(), name(name_), is_stub(true) {}

  Test(const std::string& name_, const void_callback& cb_)
    : Action(cb_), name(name_) {}

  Test(const std::string& name_, const async_callback& async_cb_)
    : Action(async_cb_), name(name_) {}
};

struct TestAccumulator {
  TestAccumulator(std::vector<Test>& tests)
    : _tests(tests) {}

  void operator()(const std::string& name) {
    _tests.emplace_back(name);
  }

  template <typename Cb>
  void operator()(const std::string& name, const Cb& cb) {
    _tests.emplace_back(name, cb);
  }

  std::vector<Test>& _tests;
};

}
