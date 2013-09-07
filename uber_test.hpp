#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <future>


namespace ut {

struct Suite;

Suite* parent = nullptr;

typedef std::function<void()> callback;
typedef std::function<void(const callback&)> registration;
typedef std::function<void(bool)> bool_callback;
typedef std::function<void(const bool_callback&)> async_callback;
typedef std::function<void(const char*, const callback&)> tagged_registration;

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

struct Suite {
  std::vector<callback> _before;
  std::vector<async_callback> _beforeAsync;
  std::vector<callback> _beforeEach;
  std::vector<async_callback> _beforeEachAsync;
  std::vector<callback> _after;
  std::vector<async_callback> _afterAsync;
  std::vector<callback> _afterEach;
  std::vector<async_callback> _afterEachAsync;

  std::vector<std::pair<std::string, callback>> _cases;
  std::vector<std::shared_ptr<Suite>> _suites;

  Suite* _parent;
  std::string _name;

  Suite() {}

  template <typename Callback>
  Suite(Suite* parent, const char* name, const Callback& describe)
    : _parent(parent), _name(name)
  {
    CallbackAccumulator before(_before, _beforeAsync);
    CallbackAccumulator beforeEach(_beforeEach, _beforeEachAsync);
    CallbackAccumulator after(_after, _afterAsync);
    CallbackAccumulator afterEach(_afterEach, _afterEachAsync);

    auto it = [&](const char* tag, const callback& cb) {
      _cases.emplace_back(std::make_pair(tag, cb));
    };

    describe(this, before, beforeEach, after, afterEach, it);

    if (_parent)
      _parent->_suites.emplace_back(std::make_shared<Suite>(*this));
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
    call(_before);
    call(_beforeAsync);

    for (const auto& c : _cases) {
      call(_beforeEach);
      call(_beforeEachAsync);
      c.second();
      call(_afterEach);
      call(_afterAsync);
    }

    call(_after);
    call(_afterAsync);

    for (const auto& s : _suites)
      s->execute();
  }
};

}

#define describe(tag) \
Suite tag = {parent, #tag, [] \
  (Suite* parent, CallbackAccumulator& before, CallbackAccumulator& beforeEach, CallbackAccumulator& after, CallbackAccumulator& afterEach, tagged_registration it) { \

#define done(tag) \
}}; \

