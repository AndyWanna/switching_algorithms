// Examples for schedulers

#include <iostream>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>

#include <scheduler/scheduler_factory.h>

#include "example_macros.h"

using namespace std;
using namespace saber;
using json = nlohmann::json;

EXAMPLE_BEGIN;

EXAMPLE("Scheduler display example") {
  vector<string> schedulers = {
      "maximum_weight", "randomized_maximal", "islip", "slqf", "ipoc", "qps", "randomized_edge_coloring", "sb_qps"
  };
    int n = 4;
    json conf = {
        {"name", "maximum_weight"},
        {"num_inputs", n},
        {"num_outputs", n}
    };
  for(const auto& name : schedulers){
    conf["name"] = name;
    auto *sched = SchedulerFactory::Create(conf);
    sched->display(std::cout);
    std::cout << "\n\n";
    delete (sched);
  }
}

EXAMPLE_END;


int main(){
  EXAMPLE_RUN;
}

