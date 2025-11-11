// Examples for traffic generator model

#include <iostream>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>

#include <traffic_generator/injection_model_factory.h>
#include <traffic_generator/traffic_pattern_factory.h>

#include "example_macros.h"

using namespace std;
using namespace saber;
using json = nlohmann::json;

EXAMPLE_BEGIN;

  EXAMPLE("That is just a dummy example");

  EXAMPLE("1.1. Bernoulli injection display example")
  {
    json inj_conf = {
        {"name", "bernoulli"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5}
    };
    auto *inj = InjectionModelFactory::Create(inj_conf);
    inj->display(std::cout);
    delete (inj);
  }

  EXAMPLE("1.2. Bernoulli injection operator<< example")
  {
    json inj_conf = {
        {"name", "bernoulli"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5}
    };
    auto *inj = InjectionModelFactory::Create(inj_conf);
    inj->operator<<(std::cout);
    std::cout << "\n";
    delete (inj);
  }

  EXAMPLE("2.1. On on injection model display example")
  {
    json inj_conf = {
        {"name", "onoff"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5},
        {"burst_size_avg", 2.0}
    };
    auto *inj = InjectionModelFactory::Create(inj_conf);
    inj->display(std::cout);

    delete (inj);
  }

  EXAMPLE("2.2. On off injection model operator<< example")
  {
    json inj_conf = {
        {"name", "onoff"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5},
        {"burst_size_avg", 2.0}
    };
    auto *inj = InjectionModelFactory::Create(inj_conf);
    inj->operator<<(std::cout);
    std::cout << "\n";

    delete (inj);
  }

  EXAMPLE("3.1. Extended on off injection model display example")
  {
    json inj_conf = {
        {"name", "extended-onoff"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5},
        {"burst_size_avg", 2.0}
    };
    auto *inj = InjectionModelFactory::Create(inj_conf);
    inj->display(std::cout);

    delete (inj);
  }

  EXAMPLE("3.2. Extended on off injection model operator<< example")
  {
    json inj_conf = {
        {"name", "extended-onoff"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5},
        {"burst_size_avg", 2.0}
    };
    auto *inj = InjectionModelFactory::Create(inj_conf);
    inj->operator<<(std::cout);
    std::cout << "\n";

    delete (inj);
  }

// Traffic Pattern
  EXAMPLE("4.1 Traffic pattern display example")
  {
    auto seed = static_cast<unsigned int>( std::chrono::system_clock::now().time_since_epoch().count());
    auto num_nodes = 8;
    json conf = {
        {"name", "uniform"},
        {"num_nodes", num_nodes},
        {"seed", seed}
    };
    std::vector<std::string> patterns = {
        "uniform", "quasi_diagonal", "log_diagonal", "diagonal"
    };
    for (const auto &name: patterns) {
      conf["name"] = name;
      auto *tp = TrafficPatternFactory::Create(conf);
      tp->display(std::cout);
      std::cout << "\n\n";
      delete (tp);
    }
  }

  EXAMPLE("4.2 Traffic pattern operator<< example")
  {
    auto seed = static_cast<unsigned int>( std::chrono::system_clock::now().time_since_epoch().count());
    auto num_nodes = 8;
    json conf = {
        {"name", "uniform"},
        {"num_nodes", num_nodes},
        {"seed", seed}
    };
    std::vector<std::string> patterns = {
        "uniform", "quasi_diagonal", "log_diagonal", "diagonal"
    };
    for (const auto &name: patterns) {
      conf["name"] = name;
      auto *tp = TrafficPatternFactory::Create(conf);
      tp->operator<<(std::cout);
      std::cout << "\n\n\n";
      delete (tp);
    }
  }

  EXAMPLE("4.3 Traffic pattern destination example")
  {
    auto seed = static_cast<unsigned int>( std::chrono::system_clock::now().time_since_epoch().count());
    auto num_nodes = 8;
    json conf = {
        {"name", "uniform"},
        {"num_nodes", num_nodes},
        {"seed", seed}
    };

    int *counter = new int[num_nodes];
    int t = int(1e6);
    const auto max_len = 80;

    std::vector<std::string> patterns = {
        "uniform", "quasi_diagonal", "log_diagonal", "diagonal"
    };

    for (const auto &name: patterns) {
      conf["name"] = name;
      auto *tp = TrafficPatternFactory::Create(conf);
      tp->display(std::cout);
      std::fill(counter, counter + num_nodes, 0);
      for (int i = 0; i < t; ++i) counter[tp->destination(0)]++;

      auto max_c = std::max_element(counter, counter + num_nodes);
      const int c_per_dot = (*max_c + max_len) / max_len;
      std::cout << "\nHow does the distribution look (numerically):\n";
      for (int s = 0; s < num_nodes; ++s) {
        std::cout << s << ": " << counter[s];
        std::cout << "\n";
      }
      std::cout << "\n";

      std::cout << "\nHow does the distribution look (\"graphically\"):\n";
      for (int s = 0; s < num_nodes; ++s) {
        std::cout << s << ": ";
        for (int i = 0; i < counter[s] / c_per_dot; ++i) std::cout << "@";
        std::cout << "\n";
      }
      std::cout << "\n\n\n";
      delete (tp);
    }

    delete[] counter;

  }

  // More examples on log diagonal (to make sure there is no numercial issue)
  EXAMPLE("4.4 Log-diagonal traffic pattern destination example")
  {
    auto seed = static_cast<unsigned int>( std::chrono::system_clock::now().time_since_epoch().count());
    auto num_nodes = 8;
    json conf = {
        {"name", "log_diagonal"},
        {"num_nodes", num_nodes},
        {"seed", seed}
    };
    for (num_nodes = 1; num_nodes <= 128; ++num_nodes) {
      conf["num_nodes"] = num_nodes;
      auto *tp = TrafficPatternFactory::Create(conf);
      tp->display(std::cout);
      std::cout << "\n\n";
      delete (tp);
    }
  }
EXAMPLE_END;

int main() {
  EXAMPLE_RUN;
  return 0;
}

