#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <stdexcept>
#include <unordered_map>

#include <ut/suite.hpp>

#include <sstream>

namespace ut {

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

#define suite(tag) \
auto tag = Registry::add(parent(), #tag, \
  [] (parent_name_getter parent, ActionAccumulator& before, ActionAccumulator& beforeEach, ActionAccumulator& after, ActionAccumulator& afterEach, TestAccumulator& it) { \

#define describe(tag) \
auto tag = Registry::add(parent(), #tag, \
  [&] (parent_name_getter parent, ActionAccumulator& before, ActionAccumulator& beforeEach, ActionAccumulator& after, ActionAccumulator& afterEach, TestAccumulator& it) { \

#define done(tag) \
});

