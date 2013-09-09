#include <uber_test.hpp>
#include <ostream_reporter.hpp>
#include <assertions.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace ut;

namespace {

describe(example1)
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
    assert_eq(*val, "1");
  });
  it("should also check value", [=] {
    std::cout << *val;
    assert_eq(*val, "3");
  });

  describe(tests)
    it("should not execute?", [] {
      std::cout << "damn";
    });

    describe(tests2)
      it("should play out", [] {
        std::cout << "hi there";
      });
      it("should throw an uncaught exception", [] {
        std::cerr << "This isn't good";
        assert(1 == 2, "1 does not equal 2");
      });
      it("should wait for 1/10 of a second", [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert(true, "Should not fail");
      });
    done(tests2)

  done(tests)
done(example1)

}
