//

#include "iq_switch_factory.h"

namespace saber {
IQSwitch *IQSwitchFactory::Create(const json &switch_conf, const json &scheduler_conf) {
  if(!(switch_conf.count("name"))) throw MissingArgumentException("Argument switch_conf MUST contain name!");
  if(!(switch_conf.count("num_inputs"))) throw MissingArgumentException("Argument switch_conf MUST contain num_inputs!");
  if(!(switch_conf.count("num_outputs")))   throw MissingArgumentException("Argument switch_conf MUST contain num_outputs!");

  std::string name = switch_conf["name"].get<std::string>();
  if (name.empty()) throw EmptyNameException("name in switch_conf CANNOT be empty!");

  int num_inputs = switch_conf["num_inputs"].get<int>();
  int num_outputs = switch_conf["num_outputs"].get<int>();

  std::string type = (switch_conf.count("type")?switch_conf["type"].get<std::string>():"generic");
  bool enable_departure_info = (switch_conf.count("enable_departure_info")?switch_conf["enable_departure_info"].get<bool>():false);
  IQSwitch *iq_switch = nullptr;

  if (type == "dummy") {
    if (!switch_conf.count("initial_queue_length")) {
      throw MissingArgumentException("Argument switch_conf MUST contain initial_queue_length for switch type \"dummy\"");
    }
    iq_switch = new DummyIQSwitch(name, num_inputs, num_outputs, switch_conf["initial_queue_length"]);
  } else if (type == "simplified") {
    // simplified IQ switch
    if (!switch_conf.count("initial_queue_length"))
        iq_switch = new sIQSwitch(name, num_inputs, num_outputs, scheduler_conf);
    else
        iq_switch = new sIQSwitch(name, num_inputs, num_outputs, switch_conf["initial_queue_length"], scheduler_conf);
  } else if (type == "generic") {
    // generic IQ switch
    iq_switch = new gIQSwitch(name, num_inputs, num_outputs, enable_departure_info, scheduler_conf);
  } else {
    throw UnknownParameterException("Unknown parameter for \"type\" in switch_conf: " + type);
  }

  return iq_switch;
}
}// namespace saber