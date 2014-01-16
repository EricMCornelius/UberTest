#pragma once

namespace ut {

struct Test;
struct Suite;

struct Reporter {
  virtual void testStarted(const Test& t) {}
  virtual void testFailed(const Test& t) {}
  virtual void testSucceeded(const Test& t) {}
  virtual void suiteStarted(const Suite& s) {}
  virtual void suiteFailed(const Suite& s) {}
  virtual void suiteSucceeded(const Suite& s) {}
};

}
