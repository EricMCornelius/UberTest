#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

// backtrace
#include <backward.hpp>

namespace ut {

struct Formatter {
  Formatter(const std::string& sep_ = " ")
    : sep(sep_) {}

  void concat_impl(std::ostream& out) {

  }

  template <typename Head, typename... Args>
  void concat_impl(std::ostream& out, Head&& h, Args&&... args) {
    out << h << sep;
    concat_impl(out, std::forward<Args>(args)...);
  }

  template <typename... Args>
  std::string concat(Args&&... args) {
    std::stringstream str;
    concat_impl(str, std::forward<Args>(args)...);
    return std::move(str.str());
  }

  template <typename Type>
  std::string join(const std::vector<Type>& elems) {
    std::stringstream str;
    for(const auto& elem : elems)
      str << elem << sep;
    return str.str();
  }

  const std::string sep = " ";
};

struct LocationInfo {
  const std::string file = "";
  const std::size_t line = 0;
  const std::string func = "";

  bool empty() {
    return file.empty() && func.empty() && line == 0;
  }
};

inline std::ostream& operator << (std::ostream& out, const LocationInfo& loc) {
  return out << "[" << loc.file << ":" << loc.line << "]:" << loc.func;
}

inline std::string stack() {
  char* ptr;
  std::size_t size = 0;
  auto handle = open_memstream(&ptr, &size);

  using namespace backward;
  StackTrace st;
  st.load_here(32);
  Printer p;
  p.print(st, handle);
  fflush(handle);
  return std::string(ptr, size);
}

struct Exception : public std::runtime_error {
  LocationInfo location;
  std::string stack;

  template <typename... Args>
  Exception(std::string&& message, LocationInfo&& location, Args&&... args)
    : std::runtime_error(ut::Formatter().concat(std::forward<Args>(args)...)),
      location(location),
      stack(ut::stack()) {}

  Exception(std::string&& message, LocationInfo&& location)
    : std::runtime_error(message),
      location(location),
      stack(ut::stack()) {}

  Exception(std::string&& message)
    : std::runtime_error(message),
      stack(ut::stack()) {}
};

template <typename... Args>
void assert(bool value, Args&&... args) {
  if (value)
    return;

  throw ut::Exception(std::string("condition failed"), std::forward<Args>(args)...);
}

template <typename... Args>
void assert(bool value, const char* message) {
  if (value)
    return;

  throw ut::Exception(message);
}

template <typename T1, typename T2, typename... Args>
void assert_eq(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 == t2)
    return;

  throw ut::Exception(ut::Formatter().concat(t1, "!=", t2), std::forward<Args>(args)...);
}

template <typename T1, typename T2, typename... Args>
void assert_neq(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 != t2)
    return;

  throw ut::Exception(ut::Formatter().concat(t1, "==", t2), std::forward<Args>(args)...);
}

template <typename T1, typename T2, typename... Args>
void assert_lt(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 < t2)
    return;

  throw ut::Exception(ut::Formatter().concat(t1, "!<", t2), std::forward<Args>(args)...);
}

template <typename T1, typename T2, typename... Args>
void assert_lte(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 <= t2)
    return;

  throw ut::Exception(ut::Formatter().concat(t1, "!<=", t2), std::forward<Args>(args)...);
}

template <typename T1, typename T2, typename... Args>
void assert_gt(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 > t2)
    return;

  throw ut::Exception(ut::Formatter().concat(t1, "!>", t2), std::forward<Args>(args)...);
}

template <typename T1, typename T2, typename... Args>
void assert_gte(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 >= t2)
    return;

  throw ut::Exception(ut::Formatter().concat(t1, "!>=", t2), std::forward<Args>(args)...);
}

}

#define ut_assert(test, ...) \
ut::assert(test, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__); \

#define ut_assert_throws(test, exception, ...) \
try { \
  test; \
  ut::assert(false, LocationInfo{__FILE__, __LINE__, __func__}, "Expected exception:", #exception, ##__VA_ARGS__); \
} \
catch(exception& e) {} \
catch(...) { \
  ut::assert(false, LocationInfo{__FILE__, __LINE__, __func__}, "Expected exception:", #exception, ##__VA_ARGS__); \
} \

#define ut_assert_eq(v1, v2, ...) \
ut::assert_eq(v1, v2, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__);

#define ut_assert_neq(v1, v2, ...) \
ut::assert_neq(v1, v2, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__);

#define ut_assert_lt(v1, v2, ...) \
ut::assert_lt(v1, v2, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__);

#define ut_assert_lte(v1, v2, ...) \
ut::assert_lte(v1, v2, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__);

#define ut_assert_gt(v1, v2, ...) \
ut::assert_gt(v1, v2, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__);

#define ut_assert_gte(v1, v2, ...) \
ut::assert_gte(v1, v2, LocationInfo{__FILE__, __LINE__, __func__}, ##__VA_ARGS__);
