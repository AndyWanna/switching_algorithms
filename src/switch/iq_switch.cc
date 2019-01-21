//

#include "iq_switch.h"

namespace saber {
// Implementation of class sIQSwitch
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
sIQSwitch::sIQSwitch(std::string name, int num_inputs, int num_outputs, const json &conf) :
    IQSwitch(std::move(name), num_inputs, num_outputs, true),
    _queue_len_mat(num_inputs, std::vector<size_t>(num_outputs, 0)) {
  _schedule = SchedulerFactory::Create(conf);
}
sIQSwitch::sIQSwitch(std::string name,
                     int num_inputs,
                     int num_outputs,
                     const IQSwitch::queue_len_mat_t &initial_queue_len,
                     const json &conf) :
    IQSwitch(name, num_inputs, num_outputs, true),
    _queue_len_mat(initial_queue_len) {
  _schedule = SchedulerFactory::Create(conf);
}
void sIQSwitch::read_inputs(){
  if (!_channel_installed) {
    std::cerr << "Please install channels first.\n";
    return;
  }
  // reset arrivals
  /*
   * To avoid clear the content of _arrivals, we use a sentry to indicate the boundary of valid content.
   *
   */
  int num_arrivals = 0;

  for (auto &ch : _input_channel) {
    while (!ch->empty()) {
      Packet *pkt = ch->receive();
      assert(pkt != nullptr && "pkt should not be null");
      assert(pkt->_source >= 0 && pkt->_source < _num_inputs);
      assert(pkt->_destination >= 0 && pkt->_destination < _num_outputs);
      _queue_len_mat[pkt->_source][pkt->_destination]++;// "enqueue" a packet
      if (num_arrivals >= _arrivals.size()) _arrivals.emplace_back(pkt->_source, pkt->_destination);
      else {
        _arrivals[num_arrivals].first = pkt->_source;
        _arrivals[num_arrivals].second = pkt->_destination;
      }
      ++ num_arrivals;
      delete(pkt); // release memory
    }
  }
  if (num_arrivals < _arrivals.size()){
    // set the sentry
    _arrivals[num_arrivals].first = -1;
    _arrivals[num_arrivals].second = -1;
  }
}
void sIQSwitch::write_outputs() {
  if (!_channel_installed) {
    std::cerr << "Please install channels first.\n";
    return;
  }
  assert(_enable_departure_info && "Departure info should be enabled");
  int num_departs = 0;
  for (int src = 0; src < _num_inputs; ++src) {
    auto dst = _schedule->match_with(src);
    // NOTE: the following check is required, because not all schedulers can guarantee to produce a "full matching",
    // _i.e.,_ some input might not have matched (to any output). And some schedulers do not perform queue length check
    // before matching a pair of input and output, hence the queue length between some matched input-output pair might be zero.
    if (dst != -1 && _queue_len_mat[src][dst] != 0) {
      assert(dst >= 0 && dst < _num_outputs);
      -- _queue_len_mat[src][dst];
      if ( num_departs >= _departures.size() ) _departures.emplace_back(src, dst);
      else {
        _departures[num_departs].first = src;
        _departures[num_departs].second = dst;
      }
      ++ num_departs;
    }
  }
  if(num_departs < _departures.size()) {
    _departures[num_departs].first = -1;
    _departures[num_departs].second = -1;
  }
}
void sIQSwitch::display(std::ostream &os) const {
  IQSwitch::display(os);
  os << "------------------------------------------------------------------------------\n";

  os << "arivals: \n";
  os << _arrivals << "\n";
  os << "departures: \n";
  os << _departures << "\n";
  os << "queue length matrix: \n";
  for(auto& row_queue_len : _queue_len_mat) {
    std::cout << row_queue_len;
    std::cout << "\n";
  }
  os << "------------------------------------------------------------\n";
  os << "Scheduler : \n";
  _schedule->display(os);
}
void sIQSwitch::reset() {
  IQSwitch::reset();
  // BugFixed: reset queue length
  for(auto& row_queue_len : _queue_len_mat) std::fill(row_queue_len.begin(), row_queue_len.end(), 0);
  _schedule->reset();
  clear();
  for (auto &_arrival : _arrivals) {
    _arrival.first = -1;
    _arrival.second = -1;
  }
  for (auto &_dep : _departures ) {
    _dep.first = -1;
    _dep.second = -1;
  }
}
// clear channel
void sIQSwitch::clear() {
  if (!_channel_installed) return;

  for (int s = 0; s < _num_inputs; ++s) {
    while (_input_channel[s] && !_input_channel[s]->empty()) {
      Packet *pkt = _input_channel[s]->receive();
      delete (pkt);
    }
  }

  for (int d = 0; d < _num_outputs; ++d) {
    while (_output_channel[d] && !_output_channel[d]->empty()) {
      Packet *pkt = _output_channel[d]->receive();
      delete (pkt);
    }
  }

}
size_t sIQSwitch::get_queue_length(int source, int destination) const {
  if(!(source >= 0 && source < _num_inputs)) throw OutOfBoundaryException("Argument source should be within [0, " + std::to_string(_num_inputs) + ").");
  if(!(destination >= 0 && destination < _num_outputs)) throw OutOfBoundaryException("Argument destination should be within [0, " + std::to_string(_num_inputs) + ").");
  return _queue_len_mat[source][destination];
}
void sIQSwitch::switching() {
  read_inputs();
  assert(_schedule != nullptr);
  _schedule->schedule(this);
  write_outputs();
}
// Implementation for class gIQSwitch
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
gIQSwitch::gIQSwitch(std::string name, int num_inputs, int num_outputs, bool enable_departure_info, const json &conf) :
    IQSwitch(name, num_inputs, num_outputs, enable_departure_info),
    _virtual_output_queue(num_inputs, std::vector<std::deque<Packet *> >(num_outputs))
{
  _schedule = SchedulerFactory::Create(conf);
  // initialize timer
  init_timer();
}

void gIQSwitch::init_timer()  {
  if(_timer != nullptr ) throw ReInitializationException("You are trying to initialize a pointer that is already initialized.");
  json conf = {
      {"name", _name + "-timer"}
  };
  _timer = Timer::New(conf);
}
void gIQSwitch::read_inputs() {
  if (!_channel_installed) throw MissingComponentException("Channels have not been installed, please install they first!");

  int num_arrivals = 0;
  for (auto &ch : _input_channel) {
    while (!ch->empty()) {
      Packet *pkt = ch->receive();
      // set arrival time
      assert(_timer != nullptr);
      pkt->set_arrival_time(_timer->get_time());

      assert(pkt != nullptr && "pkt should not be null");
      assert(pkt->_source >= 0 && pkt->_source < _num_inputs);
      assert(pkt->_destination >= 0 && pkt->_destination < _num_outputs);

      _virtual_output_queue[pkt->_source][pkt->_destination].push_back(pkt);

      if (num_arrivals < _arrivals.size()) {
        _arrivals[num_arrivals].first = pkt->get_source();
        _arrivals[num_arrivals].second = pkt->get_destination();
      } else {
        _arrivals.emplace_back(pkt->get_source(), pkt->get_destination());
      }
      ++num_arrivals;
    }
  }

  if (num_arrivals < _arrivals.size()) {
    _arrivals[num_arrivals].first = -1;// mark as end
  }
}
void gIQSwitch::write_outputs() {
  if (!_channel_installed) throw MissingComponentException("Channels have not been installed, please install they first!");
  if (_enable_departure_info) _write_outputs_depart_info_enabled();
  else _write_outputs_depart_info_disabled();
}
void gIQSwitch::_write_outputs_depart_info_disabled() {
  for (int src = 0; src < _num_inputs; ++src) {
    auto dst = _schedule->match_with(src);
    if (dst != -1 && !_virtual_output_queue[src][dst].empty()) {
      assert(dst >= 0 && dst < _num_outputs);
      Packet *pkt = _virtual_output_queue[src][dst].front();
      // set departure time
      assert(_timer != nullptr);
      pkt->set_departure_time(_timer->get_time() + _delay);
      _output_channel[dst]->send(pkt);
      _virtual_output_queue[src][dst].pop_front();// dequeue HOL packet
    }
  }
}
void gIQSwitch::_write_outputs_depart_info_enabled() {
  int num_departs = 0;
  for (int src = 0; src < _num_inputs; ++src) {
    auto dst = _schedule->match_with(src);
    if (dst != -1 && !_virtual_output_queue[src][dst].empty()) {
      assert(dst >= 0 && dst < _num_outputs);
      Packet *pkt = _virtual_output_queue[src][dst].front();
      // set departure time
      assert(_timer != nullptr);
      pkt->set_departure_time(_timer->get_time() + _delay);
      _output_channel[dst]->send(pkt);
      _virtual_output_queue[src][dst].pop_front();// dequeue HOL packet
      if(num_departs >= _departures.size()) {
        _departures.emplace_back(pkt->get_source(), pkt->get_destination());
      } else {
        _departures[num_departs].first = pkt->get_source();
        _departures[num_departs].second = pkt->get_destination();
      }
      ++ num_departs;
    }
    if (num_departs < _departures.size()) {
      _departures[num_departs].first = -1;
      _departures[num_departs].second = -1;
    }
  }
}
void gIQSwitch::display(std::ostream &os) const {
  if (_schedule == nullptr) throw MissingComponentException("Scheduler is missing.");
  if (_timer == nullptr) throw MissingComponentException("Timer is missing.");
  IQSwitch::display(os);
  os << "------------------------------------------------------------\n";
  os << "Scheduler : \n";
  _schedule->display(os);
  _timer->display(os);
}
void gIQSwitch::reset() {
  if (_schedule == nullptr) throw MissingComponentException("Scheduler is missing.");
  if (_timer == nullptr) throw MissingComponentException("Timer is missing.");
  IQSwitch::reset();
  _schedule->reset();
  _timer->reset();
  clear();
  for (auto &_arrival : _arrivals) {
    _arrival.first = -1;
    _arrival.second = -1;
  }
  for (auto &_dep : _departures ) {
    _dep.first = -1;
    _dep.second = -1;
  }
}
void gIQSwitch::clear() {
  if (!_channel_installed) return;
  for (int s = 0; s < _num_inputs; ++s) {
    for (int d = 0; d < _num_outputs; ++d) {
      while (!_virtual_output_queue[s][d].empty()) {
        Packet *pkt = _virtual_output_queue[s][d].front();
        _virtual_output_queue[s][d].pop_front();
        delete (pkt);
      }
    }
  }

  for (int s = 0; s < _num_inputs; ++s) {
    while (_input_channel[s] && !_input_channel[s]->empty()) {
      Packet *pkt = _input_channel[s]->receive();
      delete (pkt);
    }
  }

  for (int d = 0; d < _num_outputs; ++d) {
    while (_output_channel[d] && !_output_channel[d]->empty()) {
      Packet *pkt = _output_channel[d]->receive();
      delete (pkt);
    }
  }

}
void gIQSwitch::switching() {
  read_inputs();
  assert(_schedule != nullptr);
  _schedule->schedule(this);
  write_outputs();

  assert(_timer != nullptr);
  _timer->update();
}
size_t gIQSwitch::get_queue_length(int source, int destination) const {
  if(!(source >= 0 && source < _num_inputs))
    throw OutOfBoundaryException("Argument source should be within the range of [0, " + std::to_string(_num_inputs) + ").");
  if(!(destination >= 0 && destination < _num_outputs))
    throw OutOfBoundaryException("Argument destination should be within the range of [0, " + std::to_string(_num_outputs) + ").");
  return _virtual_output_queue[source][destination].size();
}
const Scheduler *const gIQSwitch::get_scheduler() const {
  return _schedule;
}
const Timer *const gIQSwitch::get_timer() const {
  return _timer;
}
} // namespace saber
