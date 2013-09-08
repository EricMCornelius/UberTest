#include <uber_test.hpp>
#include <ostream_reporter.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace ut;

describe(suite)
  auto val = std::make_shared<std::string>();
  before([]() {

  });
  before([](const bool_callback& cb) {
    cb(true);
  });
  beforeEach([=]() {
    static std::size_t count = 0;
    ++count;
    std::stringstream str;
    str << "test:" << count;
    *val = str.str();
  });
  afterEach([]() {

  });
  after([]() {

  });
  it("should do nothing", [=] {
    std::cout << *val;
  });
  it("should also do nothing", [=] {
    std::cout << *val;
  });

  describe(subsuite)
    it("should not execute?", [] {
      std::cout << "damn";
    });

    describe(subsuite2)
      it("should play out", [] {
        std::cout << "hi there";
      });
      it("should throw an uncaught exception", [] {
        std::cerr << "This isn't good";
        throw std::runtime_error("Exceptional failure");
      });
    done(subsuite2)

  done(subsuite)

  describe(subsuite3)
    it("is yet another test", [] {
      std::cout << "done";
    });
  done(subsuite3)
done(suite)


int main(int argc, char* argv[]) {
  OstreamReporter rep(std::cout);
  suite.execute(rep);
}
