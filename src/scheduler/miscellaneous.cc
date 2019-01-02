
#include "miscellaneous.h"
#include <switch/iq_switch.h>

namespace saber {
// Implementation of class RoundRobin
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////
RoundRobin::RoundRobin(std::string name, int num_inputs, int num_outputs, std::vector<int> &initial_match) :
    Scheduler(std::move(name), num_inputs, num_outputs, false),
    _initial_match(initial_match) {
  std::copy(_initial_match.begin(), _initial_match.end(), _in_match.begin());
}
void RoundRobin::schedule(const IQSwitch *sw) {
  for (int s = 0; s < _num_inputs; ++s) _in_match[s] = (_in_match[s] + 1) % _num_outputs;
}
void RoundRobin::display(std::ostream &os) const {
  Scheduler::display(os);
  os << "---------------------------------------------------------\n";
  os << "initial matching : " << _initial_match
     << "\n";
}

// Implementation of class sLQF
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////
sLQF::sLQF(std::string name, int num_inputs, int num_outputs, unsigned long seed) :
    RandomizedScheduler(std::move(name), num_inputs, num_outputs, true, seed) {
}
void sLQF::schedule(const saber::IQSwitch *sw) {
  // reset matchings in the previous time slot
  std::fill(_in_match.begin(), _in_match.end(), -1);
  std::fill(_out_match.begin(), _out_match.end(), -1);

  std::vector<int> orders(_num_inputs);
  for ( int in = 0;in < _num_inputs;++ in ) orders[in] = in;
  std::shuffle(orders.begin(), orders.end(), _eng);

  for ( int i = 0;i < _num_inputs;++ i ) {
    auto in = orders[i];
    int proposal = 0;
    for ( int out = 1;out < _num_outputs;++ out ) {
      if (sw->get_queue_length(in, out) > sw->get_queue_length(in, proposal))
        proposal = out;
    }
    if ( _out_match[proposal] == -1 || sw->get_queue_length(in,proposal) > sw->get_queue_length(_out_match[proposal], proposal))
    {
      _out_match[proposal] = in;
      _in_match[in] = proposal;
    }
  }
}
void sLQF::init(const IQSwitch *sw) {
  /* do nothing */
}
// Implementation of class iPOC
// //////////////////////////////////////////////////////////////////////////////////////////////////
iPOC::iPOC(std::string name, int num_inputs, int num_outputs, int num_iters, unsigned long seed)  :
    RandomizedScheduler(std::move(name), num_inputs, num_outputs, true, seed),
    _num_iters(num_iters),_non_empty_voqs(_num_inputs) {
}
void iPOC::display(std::ostream &os) const {
  RandomizedScheduler::display(os);
  os << "---------------------------------------------------------\n";
  os << "number of iterations : " << _num_iters << "\n";
}
void iPOC::schedule(const saber::IQSwitch *sw) {
  // reset matchings in the previous time slot
  std::fill(_in_match.begin(), _in_match.end(), -1);
  std::fill(_out_match.begin(), _out_match.end(), -1);

  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    if (std::find(_non_empty_voqs[sd.first].begin(),_non_empty_voqs[sd.first].end(), sd.second) == _non_empty_voqs[sd.first].end() ) {
      _non_empty_voqs[sd.first].push_back(sd.second);
    }
  }
  std::vector<int> orders(_num_inputs);
  for ( int in = 0;in < _num_inputs;++ in ) orders[in] = in;
  std::shuffle( orders.begin(), orders.end(), _eng);

  // TODO: multiple iterations
  for (int in : orders) {
    if (_non_empty_voqs[in].empty()) continue;
    // TODO: optimize the random selection
    std::uniform_int_distribution<> dist(0, static_cast<int>(_non_empty_voqs[in].size() - 1));
    // FIXED
    int x = _non_empty_voqs[in][dist(_eng)];
    int y = _non_empty_voqs[in][dist(_eng)];
    auto proposal = (sw->get_queue_length(in, x) > sw->get_queue_length(in, y) ? x : y);
    if ( _out_match[proposal] == -1 || sw->get_queue_length(in, proposal) > sw->get_queue_length(_out_match[proposal], proposal))
    {
      if(_out_match[proposal] != -1) _in_match[_out_match[proposal]] = -1; // mark as unmatched
      _out_match[proposal] = in;
      _in_match[in] = proposal;
    }
  }

  for (int s = 0; s < _num_inputs; ++s) {
    auto d = _in_match[s];
    if (d != -1 && sw->get_queue_length(s, d) == 1) {
      auto it = std::find(_non_empty_voqs[s].begin(),_non_empty_voqs[s].end(), d);
      _non_empty_voqs[s].erase(it);
    }
  }
}
void iPOC::init(const IQSwitch* sw) {
  for(int s = 0;s < _num_inputs;++ s)
    for(int d = 0;d < _num_outputs;++ d)
    {
      if(sw->get_queue_length(s, d) > 0)
        _non_empty_voqs[s].push_back(d);
    }
}

} // namespace saber