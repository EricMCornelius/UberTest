#include <uber_test.hpp>

namespace ut {

Suite* parent_suite() { static Suite _inst("all"); return &_inst; }

}
