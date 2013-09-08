#pragma once

#include <chrono>

namespace ut {

struct timer {
  typedef std::chrono::seconds s;
  typedef std::chrono::microseconds us;

  timer()
    : start_time(now()) { }

  std::chrono::time_point<std::chrono::system_clock> now() {
    return std::chrono::system_clock::now();
  }

  void start() {
    start_time = now();
  }

  void stop() {
    stop_time = now();
  }

  std::size_t count() {
    return std::chrono::duration_cast<us>(stop_time - start_time).count();
  }

  double seconds() {
    return count() / 1000000.0;
  }

  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> stop_time;
};

}
