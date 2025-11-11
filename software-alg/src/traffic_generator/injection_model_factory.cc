
#include "injection_model_factory.h"

namespace saber {
InjectionModel *InjectionModelFactory::Create(const json &conf) {
  std::string injection_name = conf["name"];
  InjectionModel *result = nullptr;

  if (injection_name == "bernoulli") {
    result = new BernoulliInjectionModel(conf["num_nodes"], conf["rate"], conf["seed"]);
  } else if (injection_name == "onoff" || injection_name == "extended-onoff") {
    std::vector<bool> initial;
    double alpha = (conf.count("alpha") ? conf["alpha"].get<double>() : -1.0);
    double beta = (conf.count("beta") ? conf["beta"].get<double>() : -1.0);
    double burst_size_avg = (conf.count("burst_size_avg") ? conf["burst_size_avg"].get<double>() : -1.0);
    assert ((alpha != -1 && beta == -1 && burst_size_avg == -1) ||
        (alpha == -1 && beta != -1 && burst_size_avg == -1) ||
        (alpha == -1 && beta == -1 && burst_size_avg != -1) &&
            "You can only specify exact one of the three parameters: alpha, beta, or burst_size_avg");
    if (conf.count("initial"))
      initial = conf["initial"].get<std::vector<bool> >();
    else
      initial.resize(conf["num_nodes"], false);

    if (injection_name == "onoff")
      result = new OnOffInjectionModel(conf["num_nodes"], conf["rate"], conf["seed"],
                                       alpha, beta, burst_size_avg, initial);
    else
      result = new ExtendedOnOffInjection(conf["num_nodes"], conf["rate"], conf["seed"],
                                          alpha, beta, burst_size_avg, initial);
  } else {
    std::cerr << "Unknown injection name \"" << injection_name << "\"!\n";
  }

  return result;
}
} // namespace saber