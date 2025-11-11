//

#include "simulator_factory.h"

namespace saber {

Simulator *SimulatorFactory::Create(const json &conf) {
  if(!(conf.count("name"))) throw MissingArgumentException("Argument conf MUST contain \"name\"!");
  std::string name = conf["name"].get<std::string>();
  int verbose = 0;
  if (conf.count("verbose"))
    verbose = conf["verbose"].get<int>();

  Simulator *sim = nullptr;
  // Maybe it is better to use type instead of name
  if (name == "IQSwitchSimulator") {
    if(!(conf.count("num_inputs") && conf.count("num_outputs"))) throw MissingArgumentException("num_inputs and num_outputs are required!");
    int num_inputs = conf["num_inputs"].get<int>();
    int num_outputs = conf["num_outputs"].get<int>();
    sim = new IQSwitchSimulator(name, verbose, num_inputs, num_outputs, conf);
  } else {
    throw UnknownParameterException("Unknown simulator type!");
  }

  return sim;
}
} // namespace saber