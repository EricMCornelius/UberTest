#pragma once

#include <stdexcept>
#include <sstream>

namespace ut {

template <typename Message>
void assert(bool value, const Message& msg) {
  if (value)
    return;

  std::stringstream str;
  str << msg;
  throw std::runtime_error(str.str());
}

template <typename T1, typename T2>
void assert_eq(T1 t1, T2 t2) {
  if (t1 == t2)
    return;

  std::stringstream str;
  str << t1 << " != " << t2;
  throw std::runtime_error(str.str());
}

template <typename T1, typename T2>
void assert_neq(T1 t1, T2 t2) {
  if (t1 != t2)
    return;

  std::stringstream str;
  str << t1 << " == " << t2;
  throw std::runtime_error(str.str());
}

template <typename T1, typename T2>
void assert_lt(T1 t1, T2 t2) {
  if (t1 < t2)
    return;

  std::stringstream str;
  str << t1 << " > " << t2;
  throw std::runtime_error(str.str());
}

template <typename T1, typename T2>
void assert_lte(T1 t1, T2 t2) {
  if (t1 <= t2)
    return;

  std::stringstream str;
  str << t1 << " <= " << t2;
  throw std::runtime_error(str.str());
}

template <typename T1, typename T2>
void assert_gt(T1 t1, T2 t2) {
  if (t1 != t2)
    return;

  std::stringstream str;
  str << t1 << " > " << t2;
  throw std::runtime_error(str.str());
}

template <typename T1, typename T2>
void assert_gte(T1 t1, T2 t2) {
  if (t1 != t2)
    return;

  std::stringstream str;
  str << t1 << " >= " << t2;
  throw std::runtime_error(str.str());
}

}
