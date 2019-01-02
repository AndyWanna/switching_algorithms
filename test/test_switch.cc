// test_switch.cc

// main() provided by Catch in the file test_main.cc
//

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>
#include <random>

#include <switch/iq_switch_factory.h>

using namespace saber;
using json = nlohmann::json;

TEST_CASE("Create switches should succeed", "[switch]") {
  int n = 4;
  json sched_conf = {
      {"name", "maximum_weight"},
      {"num_inputs", n},
      {"num_outputs", n}
  };
  std::vector<std::vector<size_t> > initial_queue = {
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1}
  };
  json sw_conf = {
      {"type", "simplified"},
      {"name", "test_switch"},
      {"num_inputs", n},
      {"num_outputs", n}
  };
  SECTION("Create a simplified IQ switch using MWM as its scheduler") {
    auto *sw = IQSwitchFactory::Create(sw_conf, sched_conf);
    REQUIRE(sw != nullptr);
    delete(sw);
  }
  SECTION("Create a simplified IQ switch using MWM with non-zero initial queue lengths") {
    sw_conf["initial_queue_length"] = initial_queue;
    auto *sw = IQSwitchFactory::Create(sw_conf, sched_conf);
    REQUIRE(sw != nullptr);
    delete (sw);
  }
  SECTION("Create a generic IQ switch using MWM as the scheduler"){
    sw_conf["type"] = "generic";
    auto *sw = IQSwitchFactory::Create(sw_conf, sched_conf);
    REQUIRE(sw != nullptr);
    delete (sw);    
  }
}

