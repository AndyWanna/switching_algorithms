// traffic_pattern_factory.cc file

#include "traffic_pattern_factory.h"

namespace saber {
// Implementation of Create API for class TrafficPatternFactory
TrafficPattern *TrafficPatternFactory::Create(const json &conf) {
  std::string pattern_name = conf["name"];

  TrafficPattern *result = nullptr;
  if (pattern_name == "uniform") {
    result = new UniformTrafficPattern(conf["num_nodes"], conf["seed"]);
  } else if (pattern_name == "log_diagonal") {
    result = new LogDiagonalTrafficPattern(conf["num_nodes"], conf["seed"]);
  } else if (pattern_name == "quasi_diagonal") {
    result = new QuasiDiagonalTrafficPattern(conf["num_nodes"], conf["seed"]);
  } else if (pattern_name == "diagonal") {
    result = new DiagonalTrafficPattern(conf["num_nodes"], conf["seed"]);
  } else {
    std::cerr << "Unknown pattern name \"" << pattern_name << "\"!\n";
  }

  return result;
}
} // namespace saber
