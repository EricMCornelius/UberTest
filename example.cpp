#include <uber_test.hpp>

#include <iostream>
#include <sstream>

using namespace ut;

describe(suite)
  auto val = std::make_shared<std::string>();
  before([]() {
    std::cout << "before" << std::endl;
  });
  beforeEach([=]() {
    std::cout << "beforeEach" << std::endl;
    static std::size_t count = 0;
    ++count;
    std::stringstream str;
    str << "test:" << count;
    *val = str.str();
  });
  afterEach([]() {
    std::cout << "afterEach" << std::endl;
  });
  after([]() {
    std::cout << "after" << std::endl;
  });
  it("should do nothing", [=] {
    std::cout << *val << std::endl;
  });
  it("should also do nothing", [=] {
    std::cout << *val << std::endl;
  });

  describe(subsuite)
    it("should not execute?", [] {
      std::cout << "damn" << std::endl;
    });

    describe(subsuite2)
      it("should play out", [] {
        std::cout << "hi there" << std::endl;
      });
    done(subsuite2)

  done(subsuite)

  describe(subsuite3)

  done(subsuite3)
done(suite)


int main(int argc, char* argv[]) {
  suite.execute();
}
