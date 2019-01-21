// Examples for Switch Simulator

#include <iostream>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>

#include <simulator/simulator_factory.h>

#include "example_macros.h"

using namespace std;
using namespace saber;
using json = nlohmann::json;

EXAMPLE_BEGIN;

//  EXAMPLE("1. Simulator display & simulate APIs with generic IQ switch")
//  {
//    json conf =
//        "{\n"
//        "    \"name\": \"IQSwitchSimulator\",\n"
//        "    \"num_inputs\": 8,\n"
//        "    \"num_outputs\": 8,\n"
//        "    \"least_simulation_efforts\": 100,\n"
//        "    \"most_simulation_efforts\": 200,\n"
//        "    \"load\": [0.1, 0.2, 0.3, 0.4, 0.5],\n"
//        "    \"traffic_patterns\": [\n"
//        "        \"uniform\", \"quasi_diagonal\", \"log_diagonal\", \"diagonal\"\n"
//        "    ],\n"
//        "    \"injection\": \"bernoulli\",\n"
//        "    \"switch\" : {\n"
//        "        \"name\": \"IQSwitch\",\n"
//        "        \"type\": \"generic\"\n"
//        "    },\n"
//        "    \"scheduler\": {\n"
//        "        \"name\": \"maximum_weight\"\n"
//        "    },\n"
//        "    \"seeds\": {\n"
//        "        \"traffic_pattern\": 2312418790,\n"
//        "        \"injection\": 1543682012,\n"
//        "        \"scheduler\": 1007959227\n"
//        "    }\n"
//        "}"_json;
//
//    auto *sim = SimulatorFactory::Create(conf);
//
//    sim->display(std::cout);
//    std::cout << "-------------------------------------------------------------------------\n\n";
//    sim->simulate();
//    sim->display(std::cout);
//    std::cout << "-------------------------------------------------------------------------\n\n";
//    sim->display_stats(std::cout);
//
//    delete (sim);
//
//  }

  EXAMPLE("2. Simulator display & simulate APIs with simplified IQ switch")
  {
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
        "        \"type\": \"simplified\"\n"
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

    auto *sim = SimulatorFactory::Create(conf);

    sim->display(std::cout);
    std::cout << "-------------------------------------------------------------------------\n\n";
    sim->simulate();
    sim->display(std::cout);
    std::cout << "-------------------------------------------------------------------------\n\n";
    sim->display_stats(std::cout);

    delete (sim);

  }
EXAMPLE_END;

int main() {
  EXAMPLE_RUN;
  return 0;
}
