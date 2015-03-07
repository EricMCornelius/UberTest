#include <uber_test.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace ut;

namespace {

suite(example1)
  auto val = std::make_shared<std::string>();

  // synchronous before call
  before([]() {

  });

  // asynchronous before call
  before([](const callback& cb) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cb();
  });

  // synchronous before each call
  beforeEach([=]() {
    static std::size_t count = 0;
    ++count;
    std::stringstream str;
    str << count;
    *val = str.str();
  });

  // synchronous after each call
  afterEach([]() {

  });

  // synchronous after call
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

  it("should demonstrate asynchronous failure", [](const callback& cb) {
    cb("Sample failure");
  });

  it("should demonstrate another async failure", [](const callback& cb) {
    cb(1);
  });

  describe(subsuite)
    it("should be stubbed");

    describe(subsuite2)
      it("should play out", [] {
        std::cout << "hi there";
      });

      it("should wait for 1/10 of a second", [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert(true, "Should not fail");
      });
    done(subsuite2)

  done(subsuite)

  describe(subsuite3)
    it("is yet another test", [] {
      std::cout << "done";
    });
  done(subsuite3)
done(example1)

}

int main(int argc, char* argv[]) {
  OstreamReporter rep(std::cout);
  Registry::get("root")->execute(rep);
}
