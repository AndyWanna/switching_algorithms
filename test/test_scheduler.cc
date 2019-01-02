// test_scheduler.cc

// main() provided by Catch in the file test_main.cc
//

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>
#include <random>

#include <scheduler/scheduler_factory.h>
#include <switch/iq_switch_factory.h>

using namespace saber;
using json = nlohmann::json;


TEST_CASE("1. Creating scheduler should work", "[scheduler]") {
  int n = 4;
  json sched_conf = {
      {"name", "maximum_weight"},
      {"num_inputs", n},
      {"num_outputs", n}
  };
  // First, we need a dummy switch
  std::vector<std::vector<size_t> > initial_queue = {
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1}
  };
  json sw_conf = {
      {"type", "dummy"},
      {"name", "test_switch"},
      {"num_inputs", n},
      {"num_outputs", n},
      {"initial_queue_length", initial_queue}
  };
  auto* sw = IQSwitchFactory::Create(sw_conf);
  REQUIRE(sw != nullptr);

}