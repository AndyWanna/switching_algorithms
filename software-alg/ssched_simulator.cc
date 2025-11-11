// main file for IQ switch scheduler
#include <iostream>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <fstream>

#include <simulator/simulator_factory.h>

using namespace std;
using namespace saber;
using nlohmann::json;

void usage(int argc, char *argv[]) {
  std::cout << "Usage: " << argv[0] << " configuration_file\n";
  std::cout << "\nThe configuration_file should be JSON file, for example\n";
  std::cout << "\n{\n"
               "    \"name\": \"IQSwitchSimulator\",\n"
               "    \"num_inputs\": 64,\n"
               "    \"num_outputs\": 64,\n"
               "    \"load\": [0.1, 0.2, 0.3, 0.4, 0.5],\n"
               "    \"traffic_patterns\": [\n"
               "        \"uniform\", \"quasi_diagonal\", \"log_diagonal\", \"diagonal\"\n"
               "    ],\n"
               "    \"injection\": \"bernoulli\",\n"
               "    \"switch\" : {\n"
               "        \"name\": \"IQSwitch\"\n"
               "    },\n"
               "    \"scheduler\": {\n"
               "        \"name\": \"maximum_weight\"\n"
               "    },\n"
               "    \"seeds\": {\n"
               "        \"traffic_pattern\": 2312418790,\n"
               "        \"injection\": 1543682012,\n"
               "        \"scheduler\": 1007959227\n"
               "    }\n"
               "}\n";
  std::cout << "\nYou can find more examples in the example subdirectory.\n";
  exit(0);
}

/**@brief Experiment for X vs load
 *
 * Here, "X" is the metric you care about.
 *
 * @param exp_conf          -- Configuration for the experiment
 * @param metric_name       -- The name of the metric, e.g., "delay" or "total_queue_length"
 */
void experiment_vs_load(const json &exp_conf, const std::string &metric_name) {
  Simulator *simulator = nullptr;
  json simulator_conf;
  json results;
  //const auto& tp = exp_conf["simulator"]["traffic_patterns"].get<std::vector<std::string>>();
  std::set<std::string> traffic_patterns = {
      "uniform",
      "quasi-diagonal",
      "log-diagonal",
      "diagonal"
  };


  for (json::const_iterator it = exp_conf["simulator"].cbegin(), e_it = exp_conf["simulator"].cend(); it != e_it; ++it)
    simulator_conf[it.key()] = it.value();

  for (const auto &sched_conf: exp_conf["schedulers"]) {
    std::string scheduler_name = sched_conf["name"].get<std::string>();
    simulator_conf["scheduler"] = sched_conf;

#ifdef DEBUG
    std::cerr << "Simulator configuration:\n";
    std::cerr << simulator_conf.dump(4) << "\n";
#endif
    try {
      simulator = SimulatorFactory::Create(simulator_conf);
      if (simulator != nullptr) {
        simulator->simulate();
        std::cout << "\n\nStatistic Summary:\n";
        simulator->display_stats(std::cout);
        std::cout << "\n\n";
        const auto &this_result = simulator->get_stats();
        for (json::const_iterator it = this_result.cbegin(), e_it = this_result.cend(); it != e_it; ++it) {
          if (it.key() == "experiment_name") continue;
          if (traffic_patterns.count(it.key())) {
            if (!results.count(it.key()) || !results[it.key()].count(scheduler_name)) {
              results[it.key()][scheduler_name] = std::vector<std::vector<double> >();
            }
            results[it.key()][scheduler_name].push_back(it.value()["load"]);
            if (!it.value().count(metric_name)) throw std::out_of_range("Key: " + metric_name + " was not found");
            results[it.key()][scheduler_name].push_back(it.value()[metric_name]);
          } else {
            if (!results.count(it.key())) results[it.key()] = it.value();
          }
        }
      } else {
        std::cerr << "Failed to build simulator for:\n" << simulator_conf.dump(4) << std::endl;
      }
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      if (simulator != nullptr) delete (simulator);
      exit(1);
    }
  }

  std::string out_filename;
  unsigned seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
  if (!exp_conf.count("output")) {
    out_filename = metric_name + "_vs_load_" + std::to_string(seed) + ".json";
  } else {
    out_filename = exp_conf["output"].get<std::string>() + std::to_string(seed) + ".json";
  }
  std::ofstream of(out_filename, std::ios::out);
  of << results.dump(4) << std::endl;
  of.close();
}

void experiment_vs_port(const json &exp_conf, const std::string &metric_name) {
  Simulator *simulator = nullptr;
  json simulator_conf;


  for (json::const_iterator it = exp_conf["simulator"].cbegin(), e_it = exp_conf["simulator"].cend(); it != e_it; ++it)
    simulator_conf[it.key()] = it.value();

  std::vector<double> loads = simulator_conf["load"].get<std::vector<double>>();
  for (double load: loads) {
    json results;
    for (int n : exp_conf["num_ports"]) {
      for (const auto &sched_conf: exp_conf["schedulers"]) {
        std::string scheduler_name = sched_conf["name"].get<std::string>();
        simulator_conf["scheduler"] = sched_conf;
        simulator_conf["num_inputs"] = n;
        simulator_conf["num_outputs"] = n;
        simulator_conf["load"] = std::vector<double>({load});

        try {
          simulator = SimulatorFactory::Create(simulator_conf);
          if (simulator != nullptr) {
            simulator->simulate();
            std::cout << "\n\nStatistic Summary:\n";
            simulator->display_stats(std::cout);
            std::cout << "\n\n";
            const auto &this_result = simulator->get_stats();
            for (json::const_iterator it = this_result.cbegin(), e_it = this_result.cend(); it != e_it; ++it) {
              if (!results.count(it.key())) {
                results[it.key()][scheduler_name] = std::vector<std::vector<double> >();
              }
              results[it.key()][scheduler_name][0].push_back(n);
              results[it.key()][scheduler_name][1].push_back(it.value()[metric_name].back());
            }
          } else {
            std::cerr << "Failed to build simulator for:\n" << simulator_conf.dump(4) << std::endl;
          }
        } catch (std::exception &e) {
          std::cerr << e.what() << std::endl;
          if (simulator != nullptr) delete (simulator);
          exit(1);
        }
      }
    }
    std::string out_filename;
    unsigned seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
    if (!exp_conf.count("output")) {
      out_filename = metric_name + "_vs_port_" + std::to_string(int(load * 100)) + "_" + std::to_string(seed) + ".json";
    } else {
      out_filename =
          exp_conf["output"].get<std::string>() + std::to_string(int(load * 100)) + "_" + std::to_string(seed)
              + ".json";
    }
    std::ofstream of(out_filename, std::ios::out);
    of << results.dump(4) << std::endl;
    of.close();
  }

}

void experiment_vs_burst_size(const json &exp_conf, const std::string &metric_name) {
  Simulator *simulator = nullptr;
  json simulator_conf;

  for (json::const_iterator it = exp_conf["simulator"].cbegin(), e_it = exp_conf["simulator"].cend(); it != e_it; ++it)
    simulator_conf[it.key()] = it.value();

  std::vector<double> loads = simulator_conf["load"].get<std::vector<double>>();
  for (double load: loads) {
    json results;
    for (const auto &sched_conf: exp_conf["schedulers"]) {
      std::string scheduler_name = sched_conf["name"].get<std::string>();
      simulator_conf["scheduler"] = sched_conf;
      simulator_conf["load"] = std::vector<double>({load});

      try {
        simulator = SimulatorFactory::Create(simulator_conf);
        if (simulator != nullptr) {
          simulator->simulate();
          std::cout << "\n\nStatistic Summary:\n";
          simulator->display_stats(std::cout);
          std::cout << "\n\n";
          const auto &this_result = simulator->get_stats();
          for (json::const_iterator it = this_result.cbegin(), e_it = this_result.cend(); it != e_it; ++it) {
            if (!results.count(it.key())) {
              results[it.key()][scheduler_name] = std::vector<std::vector<double> >();
            }
            results[it.key()][scheduler_name].push_back(it.value()["burst-size"]);
            results[it.key()][scheduler_name][1].push_back(it.value()[metric_name].back());
          }
        } else {
          std::cerr << "Failed to build simulator for:\n" << simulator_conf.dump(4) << std::endl;
        }
      } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        if (simulator != nullptr) delete (simulator);
        exit(1);
      }
    }
    std::string out_filename;
    unsigned seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
    if (!exp_conf.count("output")) {
      out_filename = metric_name + "_vs_burst_" + std::to_string(int(load * 100)) + "_" + std::to_string(seed) + ".json";
    } else {
      out_filename =
          exp_conf["output"].get<std::string>() + std::to_string(int(load * 100)) + "_" + std::to_string(seed)
              + ".json";
    }
    std::ofstream of(out_filename, std::ios::out);
    of << results.dump(4) << std::endl;
    of.close();
  }

}

void experiment(const json &exp_conf) {
  if (!exp_conf.count("type")) throw MissingArgumentException("Experiment type is required!");
  std::string type = exp_conf["type"].get<std::string>();

  if (type == "delay_vs_load") {
    experiment_vs_load(exp_conf, "mean-delay");
  } else if (type == "queue_length_vs_load") {
    experiment_vs_load(exp_conf, "total-queue-length-average");
  } else if (type == "delay_vs_port") {
    experiment_vs_port(exp_conf, "delay");
  } else if (type == "queue_length_vs_port") {
    experiment_vs_port(exp_conf, "total-queue-length-average");
  } else if (type == "delay_vs_burst_size") {
    experiment_vs_burst_size(exp_conf, "delay");
  } else if (type == "queue_length_vs_burst_size") {
    experiment_vs_burst_size(exp_conf, "total-queue-length-average");
  } else {
    throw UnknownParameterException("Unknown experiment type -- " + type);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) usage(argc, argv);

  std::ifstream ifs(argv[1], std::ios::in);
  if (!ifs) {
    std::cerr << "ERROR: Cannot open " << argv[1] << std::endl;
    usage(argc, argv);
  }
  auto conf = json::parse(ifs);

  experiment(conf);

  return 0;
}
