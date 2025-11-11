
#include "maximal_scheduler.h"
#include <switch/iq_switch.h>

namespace saber {
// Implementation of class RandomizedMaximalScheduler
// ////////////////////////////////////////////////////////////////////////////////////////////////
RandomizedMaximalScheduler::RandomizedMaximalScheduler(std::string name,
                                                       int num_inputs,
                                                       int num_outputs,
                                                       std::mt19937::result_type seed)
    : RandomizedScheduler(std::move(name), num_inputs, num_outputs, true, seed),
      _index(static_cast<unsigned long>(num_inputs), std::vector<int>(num_outputs, -1)) {

}
void RandomizedMaximalScheduler::insert(int source, int destination) {

  assert(source >= 0 && source < _num_inputs && "Argument source should be within the range [0, num_inputs)!");
  assert(destination >= 0 && destination < _num_outputs
             && "Argument destination should be within the range [0, num_outputs)!");

  if (_index[source][destination] == -1) {// not exist
    if (_edge_num == _edges.size()) {// need more "space"
      _edges.emplace_back(source, destination);
    } else {// just put at the end
      _edges[_edge_num].first = source;
      _edges[_edge_num].second = destination;
    }
    _index[source][destination] = _edge_num;// mark it
    ++_edge_num;
  }
}
void RandomizedMaximalScheduler::erase(int source, int destination) {
  assert(source >= 0 && source < _num_inputs && "Argument source should be within the range [0, num_inputs)!");
  assert(destination >= 0 && destination < _num_outputs
             && "Argument source should be within the range [0, num_outputs)!");

  auto i = _index[source][destination];
  auto last = _edge_num - 1;
  if (i != -1) {// exists
    assert(last >= 0);
    if (i != last) {// not the last edge
      // we just move the last edge to replace the one
      // we need to erase and then decrease edge number
      // of course we need update index accordingly.
      auto s = _edges[last].first;
      auto t = _edges[last].second;
      _edges[i].first = s;
      _edges[i].second = t;
      _index[s][t] = i;// update index of the original last edge
    }
    _index[source][destination] = -1;// mark as removed
    --_edge_num;
  }
  // do nothing
}
void RandomizedMaximalScheduler::reset() {
  _edges.clear();
  _edge_num = 0;
  for (int s = 0; s < _num_inputs; ++s) {
    for (int d = 0; d < _num_outputs; ++d) {
      _index[s][d] = -1;
    }
  }
}
void RandomizedMaximalScheduler::handle_arrivals(const saber::IQSwitch *sw) {
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;// stop check is always required
    insert(sd.first, sd.second);// try to add an edge
  }
}

void RandomizedMaximalScheduler::handle_departures(const saber::IQSwitch *sw) {
  for (int s = 0; s < _num_inputs; ++s) {
    auto d = _in_match[s];
    if (d != -1 && sw->get_queue_length(s, d) == 1) {
      erase(s, d);
    }
  }
}

void RandomizedMaximalScheduler::schedule(const saber::IQSwitch *sw) {
  assert (sw != nullptr);
  assert(_edge_num <= _edges.size());
  // arrival handling
  handle_arrivals(sw);

  // calculate new maximal matching
  std::vector<std::pair<int, int>> edges(_edges.begin(), _edges.begin() + _edge_num);

  // reset matchings in the previous time slot
  std::fill(_in_match.begin(), _in_match.end(), -1);
  std::fill(_out_match.begin(), _out_match.end(), -1);

  if (_edge_num == 0) return;
  // shuffle the edges randomly O(|E|)
  std::shuffle(edges.begin(), edges.end(), _eng);

  // get the maximal matching O(|E|)
  int match_size = 0;
  for (const auto &e : edges) {
    if (_in_match[e.first] == -1 &&
        _out_match[e.second] == -1) {
      // it is OK to add to current matching
      _in_match[e.first] = e.second;
      _out_match[e.second] = e.first;
      ++match_size;
      //reach maximal?
      if (match_size == std::min(_num_outputs, _num_inputs))
        break;
    }
  }
  // departure handling
  handle_departures(sw);
}
void RandomizedMaximalScheduler::init(const IQSwitch *sw) {
  for(int src = 0;src < _num_inputs;++ src)
    for(int dst = 0; dst < _num_outputs;++ dst)
      if(sw->get_queue_length(src, dst) > 0) insert(src, dst);
}

// Implementation of class iSLIP
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
iSLIP::iSLIP(const std::string &name,
             int num_inputs,
             int num_outputs,
             int iterations,
             const std::vector<int> &grant_pointers,
             const std::vector<int> &accept_pointer) :
    Scheduler(name, num_inputs, num_outputs, true), _iterations(iterations),
    _grant_pointers(grant_pointers), _accept_pointer(accept_pointer),
    _queue_status(static_cast<unsigned long>(num_inputs),
                  std::vector<bool>(static_cast<unsigned long>(num_outputs), false)) {
  assert (iterations > 0 && "Argument iterations should be a positive integer");
  assert(_grant_pointers.size() == num_outputs);
  assert(_accept_pointer.size() == num_inputs);
}
void iSLIP::reset() {
  std::fill(_grant_pointers.begin(), _grant_pointers.end(), 0);
  std::fill(_accept_pointer.begin(), _accept_pointer.end(), 0);
  // previous bug: forgot to reset queue status
  for (int s = 0; s < _num_inputs; ++s) {
    for (int d = 0; d < _num_outputs; ++d) {
      _queue_status[s][d] = false;
    }
  }
}
void iSLIP::display(std::ostream &os) const  {
  Scheduler::display(os);
  os << "-------------------------------------------------------------\n";
  os <<   "iterations     : " << _iterations
     << "\ngrant pointer  : " << _grant_pointers
     << "\naccept pointer : " << _accept_pointer
     << "\n";
}
void iSLIP::handle_arrivals(const saber::IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    _queue_status[sd.first][sd.second] = true;// non-empty
  }
}

void iSLIP::handle_departures(const saber::IQSwitch *sw) {
  for (int s = 0; s < _num_inputs; ++s) {
    auto d = _in_match[s];
    if (d != -1 && sw->get_queue_length(s, d) == 1) {
      _queue_status[s][d] = false;
    }
  }
}

void iSLIP::schedule(const saber::IQSwitch *sw) {
  assert (sw != nullptr);
  // arrival handling
  handle_arrivals(sw);

  // reset matchings in the previous time slot
  std::fill(_in_match.begin(), _in_match.end(), -1);
  std::fill(_out_match.begin(), _out_match.end(), -1);

  std::vector<int> check(_num_outputs, -1);

  for (int it = 0; it < _iterations; ++it) {
    // Request & Grant
    std::fill(check.begin(), check.end(), -1);
    //
    for (int out = 0; out < _num_outputs; ++out) {
      if (_out_match[out] != -1) continue;// already matched
      int input_offset = _grant_pointers[out];
      for (int in = 0; in < _num_inputs; ++in) {
        int input = (in + input_offset) % _num_inputs;
        if (_in_match[input] == -1 && _queue_status[input][out]) {
          check[out] = input;
//          if (it == 0)_grant_pointers[out] = (input + 1) % _num_inputs;
          break;
        }
      }
    }

    // Accept
    //std::fill(accepts.begin(), accepts.end(), -1);
    for (int in = 0; in < _num_inputs; ++in) {
      if (_in_match[in] != -1) continue;// already matched
      int output_offset = _accept_pointer[in];
      for (int out = 0; out < _num_outputs; ++out) {
        int output = (out + output_offset) % _num_outputs;
        if (check[output] == in) {
          assert(_in_match[in] == -1 && _out_match[output] == -1);
          _in_match[in] = output;
          _out_match[output] = in;
          if (it == 0) {// update
            _accept_pointer[in] = (output + 1) % _num_outputs;
            _grant_pointers[output] = (in + 1) % _num_inputs;
          }
          break;
        }
      }
    }
  }

  handle_departures(sw);
}
void iSLIP::init(const IQSwitch *sw) {
  for (int s = 0; s < _num_inputs; ++s)
    for(int d = 0;d < _num_outputs;++ d)
      _queue_status[s][d] = sw->get_queue_length(s, d) != 0;
}

}// namespace saber


