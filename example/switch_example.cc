// Examples for IQ switches

#include <iostream>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>

#include <switch/iq_switch_factory.h>

#include "example_macros.h"

using namespace std;
using namespace saber;
using json = nlohmann::json;

EXAMPLE_BEGIN;

EXAMPLE("IQ switch display example"){
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
    
    vector<string> switches = {
        "simplified", "generic"
    };
    
    for(const auto& type : switches) {
      sw_conf["type"] = type;
      auto *sw = IQSwitchFactory::Create(sw_conf, sched_conf);
      sw->display(cout);
      cout << "\n\n";
      delete(sw);
    }
}

EXAMPLE_END;


int main(){
  EXAMPLE_RUN;
  return 0;
}