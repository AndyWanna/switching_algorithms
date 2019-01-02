
#include "max_weight_scheduler.h"
#include <switch/iq_switch.h>

namespace saber {
// Implementation of class MaxWeightScheduler
// /////////////////////////////////////////////////////////////////////////////////
MaxWeightScheduler::MaxWeightScheduler(std::string name, int num_inputs, int num_outputs, bool out_match_enabled)
    : Scheduler(std::move(name), num_inputs, num_outputs, out_match_enabled), _weight(_g) {
  // construct graph
  for (int u = 0; u < num_inputs; ++u)
    _left.push_back(_g.addNode());
  for (int v = 0; v < num_outputs; ++v)
    _right.push_back(_g.addNode());

  initialize();
}
void MaxWeightScheduler::update_in_match(const MaxWeightScheduler::MaxWeightedMatching &mwm) {
  std::fill(_in_match.begin(), _in_match.end(), -1);
  Node u, v;
  /* read results */
  for (int i = 0; i < _num_inputs; ++i) {
    u = _left[i];
    v = mwm.mate(u);
    if (v == INVALID)
      _in_match[i] = -1; /* make sure every i is setting */
    else {
      assert(_g.id(v) >= _num_inputs && "Any input's mate should be an output!");
      _in_match[i] = _g.id(v) - _num_inputs;
    }
  }
}
void MaxWeightScheduler::update_out_match(const MaxWeightScheduler::MaxWeightedMatching &mwm) {
  if (!_out_match_enabled)
    return;
  std::fill(_out_match.begin(), _out_match.end(), -1);
  Node u, v;
  /* read results */
  for (int i = 0; i < _num_outputs; ++i) {
    u = _right[i];
    v = mwm.mate(u);
    if (v == INVALID)
      _out_match[i] = -1; /* make sure every i is setting */
    else {
      assert(_g.id(v) < _num_inputs && "Any output's mate should be an input!");
      _out_match[i] = _g.id(v);
    }
  }
}
void MaxWeightScheduler::initialize() {
  Edge e;
  _edges.resize(static_cast<unsigned long>(_num_inputs), std::vector<int>(_num_outputs, -1));

  for (int i = 0; i < _num_inputs; ++i) {
    for (int j = 0; j < _num_outputs; ++j) {
      e = _g.addEdge(_left[i], _right[j]);
      _weight[e] = 0;
      _edges[i][j] = _g.id(e);
    }
  }
  _graph_initialized = true;
}
void MaxWeightScheduler::reset_weight() {
  assert (_edges.size() == _num_inputs);
  Edge e;
  int id;
  for (int i = 0; i < _num_inputs; ++i) {
    for (int j = 0; j < _num_outputs; ++j) {
      id = _edges[i][j];
      e = _g.edgeFromId(id);
      _weight[e] = 0;
    }
  }
}
void MaxWeightScheduler::update(int source, int destination, int new_weight) {
  assert(source >= 0 && source < _num_inputs && "Argument source should be within the range [0, num_inputs)!");
  assert(source >= 0 && source < _num_outputs && "Argument source should be within the range [0, num_outputs)!");
  assert(new_weight >= 0 && "Argument new_weight should be a non-negative integer!");

  assert(_graph_initialized && "Graph should be initialized!");

  Edge e = _g.edgeFromId(_edges[source][destination]);
  _weight[e] = new_weight;
}
void MaxWeightScheduler::reset()  {
  _graph_initialized = false;
  reset_weight();
  _graph_initialized = true;
}
void MaxWeightScheduler::handle_arrivals(const IQSwitch *sw) {
  assert(_graph_initialized && "Graph MUST be initialized");
  auto arrivals = sw->get_arrivals();

  Edge e;
  int id;
  for (const auto &sd : arrivals) {
    if (sd.first == -1)
      break;
    else {
      id = _edges[sd.first][sd.second];
      e = _g.edgeFromId(id);
      ++_weight[e];

      //assert(_weight[e] == sw->get_queue_length(sd.first, sd.second) && "Weight MUST be consistent");
    }
  }
}

void MaxWeightScheduler::handle_departures(const IQSwitch *sw) {
    assert(_graph_initialized && "Graph MUST be initialized");
    int d;
    Edge e;
    int id;
    for (int s = 0; s < _num_inputs; ++s) {
      d = _in_match[s];
      if (d != -1 && sw->get_queue_length(s, d) > 0) {

        id = _edges[s][d];
        e = _g.edgeFromId(id);

        assert (_weight[e] == sw->get_queue_length(s, d) && "Weight MUST be consistent");
        --_weight[e];
      }
    }
}
void MaxWeightScheduler::schedule(const IQSwitch *sw) {
  assert(_graph_initialized && "Graph should be initialized!");
  MaxWeightedMatching mwm(_g, _weight);
  handle_arrivals(sw);
  mwm.run();
  update_in_match(mwm);
  handle_departures(sw);
  if (_out_match_enabled)
    update_out_match(mwm);
}
void MaxWeightScheduler::init(const IQSwitch *sw) {
  Edge e;
  int id;
  for (int i = 0; i < _num_inputs; ++i) {
    for (int j = 0; j < _num_outputs; ++j) {
      id = _edges[i][j];
      e = _g.edgeFromId(id);
      _weight[e] = sw->get_queue_length(i, j);
    }
  }
}
} // namespace saber

