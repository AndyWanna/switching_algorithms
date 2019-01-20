// test_switch.cc

// main() provided by Catch in the file test_main.cc
//

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>
#include <random>

#include <simulator/simulator_factory.h>

using namespace saber;
using json = nlohmann::json;




TEST_CASE("1. Creating a simple simulator should succeed", "[simulator]") {
  json conf =
      "{\n"
      "    \"name\": \"IQSwitchSimulator\",\n"
      "    \"num_inputs\": 8,\n"
      "    \"num_outputs\": 8,\n"
      "    \"least_simulation_efforts\": 100,\n"
      "    \"most_simulation_efforts\": 200,\n"
      "    \"load\": [0.1, 0.2, 0.3, 0.4, 0.5],\n"
      "    \"traffic_patterns\": [\n"
      "        \"uniform\", \"quasi_diagonal\", \"log_diagonal\", \"diagonal\"\n"
      "    ],\n"
      "    \"injection\": \"bernoulli\",\n"
      "    \"switch\" : {\n"
      "        \"name\": \"IQSwitch\",\n"
      "        \"type\": \"generic\"\n"
      "    },\n"
      "    \"scheduler\": {\n"
      "        \"name\": \"maximum_weight\"\n"
      "    },\n"
      "    \"seeds\": {\n"
      "        \"traffic_pattern\": 2312418790,\n"
      "        \"injection\": 1543682012,\n"
      "        \"scheduler\": 1007959227\n"
      "    }\n"
      "}"_json;

  SECTION("Simulator with a generic IQ switch") {
    auto* sim = SimulatorFactory::Create(conf);

    REQUIRE(sim != nullptr);
    delete(sim);
  }
  SECTION("Simulator with a simplified IQ switch") {
    conf["switch"]["type"] = "simplified";
    auto* sim = SimulatorFactory::Create(conf);

    REQUIRE(sim != nullptr);
    delete(sim);
  }
}
