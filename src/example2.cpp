#include <uber_test.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace ut;

namespace {

describe(example2)
  auto val = std::make_shared<std::string>();

  before([]() {

  });

  before([](const callback& cb) {
    // demonstrate a pause in asynchronous before call
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cb();
  });

  beforeEach([=]() {
    static std::size_t count = 0;
    ++count;
    std::stringstream str;
    str << count;
    *val = str.str();
  });

  afterEach([]() {

  });

  after([]() {

  });

  it("should check value", [=] {
    std::cout << *val;
    ut_assert_eq(*val, "1");
  });

  it("should also check value", [=] {
    std::cout << *val;
    ut_assert_eq(*val, "3");
  });

  describe(tests)
    it("should throw an uncaught exception", [] {
      ut_assert(1 == 2, "1 does not equal 2");
    });
  done(tests)
done(example1)

}
