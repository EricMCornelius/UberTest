#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace ut {

struct Suite;

Suite* parent = nullptr;

typedef std::function<void()> callback;
typedef std::function<void(const callback&)> registration;
typedef std::function<void(const char*, const callback&)> tagged_registration;

struct Suite {
  std::vector<callback> _before;
  std::vector<callback> _beforeEach;
  std::vector<callback> _after;
  std::vector<callback> _afterEach;
  std::vector<std::pair<std::string, callback>> _cases;
  std::vector<std::shared_ptr<Suite>> _suites;
  Suite* _parent;
  std::string _name;

  Suite() {}

  template <typename Callback>
  Suite(Suite* parent, const char* name, const Callback& c) 
    : _parent(parent), _name(name)
  {
    auto before = [&](const callback& cb) {
      _before.emplace_back(cb);
    };
    auto beforeEach = [&](const callback& cb) {
      _beforeEach.emplace_back(cb);
    };
    auto after = [&](const callback& cb) {
      _after.emplace_back(cb);
    };
    auto afterEach = [&](const callback& cb) {
      _afterEach.emplace_back(cb);
    };
    auto it = [&](const char* tag, const callback& cb) {
      _cases.emplace_back(std::make_pair(tag, cb));
    };

    c(this, before, beforeEach, after, afterEach, it);

    if (_parent)
      _parent->_suites.emplace_back(std::make_shared<Suite>(*this));
  }

  void execute() const {
    for (const auto& b : _before)
      b();
    for (const auto& c : _cases) {
      for (const auto& be : _beforeEach)
        be();
      c.second();
      for (const auto& ae : _afterEach)
        ae();
    }
    for (const auto& a : _after)
      a();
    for (const auto& s : _suites)
      s->execute();
  }
};

}

#define describe(tag) \
Suite tag = {parent, #tag, [] \
  (Suite* parent, registration before, registration beforeEach, registration after, registration afterEach, \
   tagged_registration it) { \

#define done(tag) \
}}; \

