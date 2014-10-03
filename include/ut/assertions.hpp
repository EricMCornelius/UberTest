#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

// backtrace
#include <execinfo.h>
#include <cxxabi.h>

namespace ut {

struct TraceInfo {
  TraceInfo(const std::string& binary_, const std::string& symbol_raw_, const std::string& address_)
    : binary(binary_), symbol_raw(symbol_raw_), address(address_)
  {
    int status;
    auto realname = abi::__cxa_demangle(symbol_raw.data(), 0, 0, &status);
    if (status == 0) {
      symbol = realname;
      free(realname);
    }
    else {
      symbol = symbol_raw;
    }
  }

  std::string binary;
  std::string symbol_raw;
  std::string address;

  std::string symbol;
};

inline std::ostream& operator << (std::ostream& out, const TraceInfo& info) {
  auto pad = 120 - info.symbol.size();
  if (pad < 0)
    pad = 0;
  return out << info.symbol << std::string(pad, ' ') << "[" << std::setw(14) << info.address << "] " << info.binary;
}

inline std::vector<std::string> raw_trace() {
  void* funcs[128];
  std::size_t num_funcs = backtrace(funcs, 128);

  auto sym = backtrace_symbols(funcs, num_funcs);
  std::vector<std::string> results;

  for (auto idx = 0u; idx < num_funcs; ++idx)
    results.emplace_back(sym[idx]);

  return std::move(results);
}

inline std::vector<TraceInfo> trace() {
  const auto& raw = raw_trace();
  std::vector<TraceInfo> results;

  for (const auto& line : raw) {
    auto symbol_start = line.find_first_of("(");
    auto symbol_end = line.find_first_of(")+", symbol_start);
    auto address_start = line.find_first_of("[", symbol_end);
    auto address_end = line.find_first_of("]", address_start);

    results.emplace_back(line.substr(0, symbol_start), line.substr(symbol_start + 1, symbol_end - symbol_start - 1), line.substr(address_start + 1, address_end - address_start - 1));
  }
  return results;
}

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

  const std::string& sep = " ";
};

inline std::string stack() {
  const auto& lines = trace();
  std::vector<TraceInfo> drop(std::begin(lines) + 3, std::end(lines));
  return "\n" + ut::Formatter("\n").join(drop);
}

struct LocationInfo {
  const char* file;
  const std::size_t line;
  const char* func;
};

inline std::ostream& operator << (std::ostream& out, const LocationInfo& loc) {
  return out << "[" << loc.file << ":" << loc.line << "]:" << loc.func;
}

template <typename... Args>
void assert(bool value, Args&&... args) {
  if (value)
    return;

  throw std::runtime_error(ut::Formatter().concat(std::forward<Args>(args)...));
}

template <typename T1, typename T2, typename... Args>
void assert_eq(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 == t2)
    return;

  throw std::runtime_error(ut::Formatter().concat(t1, "!=", t2, std::forward<Args>(args)...));
}

template <typename T1, typename T2, typename... Args>
void assert_neq(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 != t2)
    return;

  throw std::runtime_error(ut::Formatter().concat(t1, "==", t2, std::forward<Args>(args)...));
}

template <typename T1, typename T2, typename... Args>
void assert_lt(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 < t2)
    return;

  throw std::runtime_error(ut::Formatter().concat(t1, "!<", t2, std::forward<Args>(args)...));
}

template <typename T1, typename T2, typename... Args>
void assert_lte(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 <= t2)
    return;

  throw std::runtime_error(ut::Formatter().concat(t1, "!<=", t2, std::forward<Args>(args)...));
}

template <typename T1, typename T2, typename... Args>
void assert_gt(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 > t2)
    return;

  throw std::runtime_error(ut::Formatter().concat(t1, "!>", t2, std::forward<Args>(args)...));
}

template <typename T1, typename T2, typename... Args>
void assert_gte(T1&& t1, T2&& t2, Args&&... args) {
  if (t1 >= t2)
    return;

  throw std::runtime_error(ut::Formatter().concat(t1, "!>=", t2, std::forward<Args>(args)...));
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
