
#include "scheduler.h"
#include <switch/iq_switch.h>

namespace saber{
// Implementation of class Scheduler
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int Scheduler::ID = 0;
Scheduler::Scheduler(std::string name, int num_inputs, int num_outputs, bool out_match_enabled) :
    _id(ID++),
    _name(std::move(name)), _num_inputs(num_inputs), _num_outputs(num_outputs), _out_match_enabled(out_match_enabled),
    _in_match(num_inputs, -1) {
  assert (!_name.empty() && "Argument name cannot be empty ");
  assert(num_inputs > 0 && num_inputs <= MAX_PORTS
             && ("Argument num_inputs should be a positive integer no larger than MAX_PORTS."));
  assert(num_outputs > 0 && num_outputs <= MAX_PORTS
             && ("Argument num_inputs should be a positive integer no larger than MAX_PORTS."));

  if (out_match_enabled)
    _out_match.resize(static_cast<unsigned long>(num_outputs), -1);
}
void Scheduler::display(std::ostream &os) const {
  os <<   "name               : " << _name
     << "\nid                 : " << _id
     << "\nnum_inputs         : " << _num_inputs
     << "\nnum_outputs        : " << _num_outputs
     << "\nout_match_enabled  : " << _out_match_enabled
     << "\nin_match           : " << _in_match
     << "\nout_match          : " << _out_match
     << "\n";
}
int Scheduler::match_with(int source, const PortType &pt) const {
  switch (pt) {
    case PortType::IN :return in_match_with(source);
    case PortType::OUT :return out_match_with(source);
    default: {
      return in_match_with(source); // this line should not be accessed
    }
  }
}
int Scheduler::in_match_with(int source) const {
  assert (source >= 0 && source < _num_inputs
              && "Argument source should be an integer in the range [0, num_inputs).");
  return _in_match[source];
}
int Scheduler::out_match_with(int dest) const {
  assert (dest >= 0 && dest < _num_outputs
              && "Argument source should be an integer in the range [0, num_outputs).");
  if (_out_match_enabled) return _out_match[dest];
  else {
    auto target = std::find(_in_match.begin(), _in_match.end(), dest);
    return (target == _in_match.end() ? -1 : (*target));
  }
}

// Implementation of class RandomizedScheduler
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RandomizedScheduler::RandomizedScheduler(std::string name,
                                         int num_inputs,
                                         int num_outputs,
                                         bool out_match_enabled,
                                         std::mt19937::result_type seed)
    : Scheduler(std::move(name), num_inputs, num_outputs, out_match_enabled), _seed(seed), _eng(seed) {
}
void RandomizedScheduler::display(std::ostream &os) const {
  Scheduler::display(os);
  os <<   "seed               : " << _seed << "\n";
}
}
