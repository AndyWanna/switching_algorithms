
#include "sb_qps.h"
#include <switch/iq_switch.h>
#include <queue> // std::priority_queue
#include <functional>

namespace saber {
// Implementation of class QB_QPS_HalfHalf
SB_QPS_HalfHalf_Oblivious::SB_QPS_HalfHalf_Oblivious(std::string name,
                                                     int num_inputs,
                                                     int num_outputs,
                                                     int frame_size,
                                                     std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)) {
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_Oblivious::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
}

void SB_QPS_HalfHalf_Oblivious::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_HalfHalf_Oblivious::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
    --_cf_packets_counter[s][d];
  }
}

int SB_QPS_HalfHalf_Oblivious::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_HalfHalf_Oblivious::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);

  // maximum number of accepts for each time slots
  int max_accepts = (((frame_id + 1) * 2 > frame_size()) ? 2 : 1);

  std::vector<std::array<int, 2> > out_accepts(num_outputs());
  for (auto &oac: out_accepts) oac.fill(-1);

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 && out_accepts[out][1] != -1) {
     // if (sw->get_queue_length(in, out) > sw->get_queue_length(out_accepts[out][0], out)) {
     if(_cf_packets_counter[in][out] > _cf_packets_counter[out_accepts[out][0]][out]){
        out_accepts[out][1] = out_accepts[out][0];
        out_accepts[out][0] = in;
      } else if (_cf_packets_counter[in][out] > _cf_packets_counter[out_accepts[out][1]][out]) {
        out_accepts[out][1] = in;
      }
    } else {
      if (out_accepts[out][0] == -1) out_accepts[out][0] = in;
      else {
        if (_cf_packets_counter[in][out] > _cf_packets_counter[out_accepts[out][0]][out]) {
          out_accepts[out][1] = out_accepts[out][0];
          out_accepts[out][0] = in;
        } else {
          out_accepts[out][1] = in;
        }
      }
    }

  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out][0] == -1) continue;
    if (max_accepts > 1 && out_accepts[out][1] != -1) {
      int in = out_accepts[out][1];
      // available only when both available
      auto mf = (_match_flag_in[in] | _match_flag_out[out]);
      for (int f = (int) (frame_id) - 1; f >= 0; --f) {
        if (!mf.test(f)) {
          _match_flag_in[in].set(f);
          _match_flag_out[out].set(f);
          assert(_schedules[f][in] == -1);
          _schedules[f][in] = out;
          vdep.emplace_back(in, out);
          break;// pay special attention (first fit)
        }
      }
    }
    int in = out_accepts[out][0];
    _match_flag_in[in].set(frame_id);
    _match_flag_out[out].set(frame_id);
    _schedules[frame_id][in] = out;
    vdep.emplace_back(in, out);
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}

void SB_QPS_HalfHalf_Oblivious::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_HalfHalf_Oblivious::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_HalfHalf_Oblivious::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
//    for (size_t in = 0; in < num_inputs(); ++in) sched[in] = in;
//    std::shuffle(sched.begin(), sched.end(), _eng);
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_Oblivious::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}

// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_HalfHalf_AvailabilityAware
SB_QPS_HalfHalf_AvailabilityAware::SB_QPS_HalfHalf_AvailabilityAware(std::string name, int num_inputs, int num_outputs, int frame_size,
    std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _output_availability(num_outputs, 0),
    _input_availability(num_inputs, 0){
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_AvailabilityAware::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
  std::fill(_input_availability.begin(), _input_availability.end(), 0);
  std::fill(_output_availability.begin(), _output_availability.end(), 0);
}

void SB_QPS_HalfHalf_AvailabilityAware::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_HalfHalf_AvailabilityAware::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
    --_cf_packets_counter[s][d];
  }
}

int SB_QPS_HalfHalf_AvailabilityAware::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_HalfHalf_AvailabilityAware::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);

  // maximum number of accepts for each time slots
  int max_accepts = (((frame_id + 1) * 2 > frame_size()) ? (num_inputs()) : 1);

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }

  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
    if (max_accepts > 1) {
      // sort proposals based on VOQ lengths
      // actually we can optimize by move sorting inside the if check
      // and whenever if fails, we just find the largest element and
      // move it to the beginning of the vector!
      std::sort(out_accepts[out].begin(), out_accepts[out].end(), [=](const int in1, const int in2){
        return _cf_packets_counter[in1][out] > _cf_packets_counter[in2][out];
      });

      for (int i = 1; i < std::min(max_accepts, (int)out_accepts[out].size()) && _output_availability[out] > 0; ++i) {
        int in = out_accepts[out][i];
        if (_input_availability[in] == 0) continue; // no available ts
        // available only when both available
        auto mf = (_match_flag_in[in] | _match_flag_out[out]);
        for (int f = (int) (frame_id) - 1; f >= 0; --f) {
          if (!mf.test(f)) {
            _match_flag_in[in].set(f);
            _match_flag_out[out].set(f);
            -- _input_availability[in];
            -- _output_availability[out];
            assert(_schedules[f][in] == -1);
            _schedules[f][in] = out;
            vdep.emplace_back(in, out);
            break;// pay special attention (first fit)
          }
        }
      }
    }

    int in = out_accepts[out][0];
    _match_flag_in[in].set(frame_id);
    _match_flag_out[out].set(frame_id);
    _schedules[frame_id][in] = out;
    out_match[out] = in;
    vdep.emplace_back(in, out);
  }

  for (int in = 0;in < num_inputs();++ in) {
    if (_schedules[frame_id][in] == -1) {
      // not matched in this time slot
      _input_availability[in] ++;
    }
  }

  for (int out = 0;out < num_outputs();++ out){
    if (out_match[out] == -1){
      // not matched
      _output_availability[out] ++;
    }
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}

void SB_QPS_HalfHalf_AvailabilityAware::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_HalfHalf_AvailabilityAware::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_HalfHalf_AvailabilityAware::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
//    for (size_t in = 0; in < num_inputs(); ++in) sched[in] = in;
//    std::shuffle(sched.begin(), sched.end(), _eng);
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_AvailabilityAware::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}

// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_Adaptive
SB_QPS_Adaptive::SB_QPS_Adaptive(std::string name, int num_inputs, int num_outputs, int frame_size,
                                                                     std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _output_availability(num_outputs, 0),
    _input_availability(num_inputs, 0){
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_Adaptive::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
  std::fill(_input_availability.begin(), _input_availability.end(), 0);
  std::fill(_output_availability.begin(), _output_availability.end(), 0);
}

void SB_QPS_Adaptive::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_Adaptive::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
    --_cf_packets_counter[s][d];
  }
}

int SB_QPS_Adaptive::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_Adaptive::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);

  // maximum number of accepts for each time slots
  std::vector<int> max_accepts(num_outputs(), 1);
  for(int out = 0;out < num_outputs();++ out)
    max_accepts[out] += int(std::round((double)_output_availability[out] / (frame_size() - frame_id)));

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts[out] > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }

  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
    if (max_accepts[out] > 1) {
      // sort proposals based on VOQ lengths
      // actually we can optimize by move sorting inside the if check
      // and whenever if fails, we just find the largest element and
      // move it to the beginning of the vector!
      std::sort(out_accepts[out].begin(), out_accepts[out].end(), [=](const int in1, const int in2){
        return _cf_packets_counter[in1][out] > _cf_packets_counter[in2][out];
      });

      for (int i = 1; i < std::min(max_accepts[out], (int)out_accepts[out].size()) && _output_availability[out] > 0; ++i) {
        int in = out_accepts[out][i];
        if (_input_availability[in] == 0) continue; // no available ts
        // available only when both available
        auto mf = (_match_flag_in[in] | _match_flag_out[out]);
        for (int f = (int) (frame_id) - 1; f >= 0; --f) {
          if (!mf.test(f)) {
            _match_flag_in[in].set(f);
            _match_flag_out[out].set(f);
            -- _input_availability[in];
            -- _output_availability[out];
            assert(_schedules[f][in] == -1);
            _schedules[f][in] = out;
            vdep.emplace_back(in, out);
            break;// pay special attention (first fit)
          }
        }
      }
    }

    int in = out_accepts[out][0];
    _match_flag_in[in].set(frame_id);
    _match_flag_out[out].set(frame_id);
    _schedules[frame_id][in] = out;
    out_match[out] = in;
    vdep.emplace_back(in, out);
  }

  for (int in = 0;in < num_inputs();++ in) {
    if (_schedules[frame_id][in] == -1) {
      // not matched in this time slot
      _input_availability[in] ++;
    }
  }

  for (int out = 0;out < num_outputs();++ out){
    if (out_match[out] == -1){
      // not matched
      _output_availability[out] ++;
    }
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}

void SB_QPS_Adaptive::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_Adaptive::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_Adaptive::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
//    for (size_t in = 0; in < num_inputs(); ++in) sched[in] = in;
//    std::shuffle(sched.begin(), sched.end(), _eng);
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_Adaptive::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}
// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_Basic
SB_QPS_Basic::SB_QPS_Basic(std::string name, int num_inputs, int num_outputs, int frame_size,
                                 std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0))
    {
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_Basic::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
}

void SB_QPS_Basic::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_Basic::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
    --_cf_packets_counter[s][d];
  }
}

int SB_QPS_Basic::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_Basic::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);

  // maximum number of accepts for each time slots
  int max_accepts = 1;

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }

  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
    int in = out_accepts[out][0];
    _match_flag_in[in].set(frame_id);
    _match_flag_out[out].set(frame_id);
    _schedules[frame_id][in] = out;
    out_match[out] = in;
    vdep.emplace_back(in, out);
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}

void SB_QPS_Basic::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_Basic::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_Basic::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
//    for (size_t in = 0; in < num_inputs(); ++in) sched[in] = in;
//    std::shuffle(sched.begin(), sched.end(), _eng);
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_Basic::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}


// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_HalfHalf_MI
SB_QPS_HalfHalf_MI::SB_QPS_HalfHalf_MI(std::string name, int num_inputs, int num_outputs, int frame_size,
                                       std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _output_availability(num_outputs, 0),
    _input_availability(num_inputs, 0){
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_MI::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
  std::fill(_input_availability.begin(), _input_availability.end(), 0);
  std::fill(_output_availability.begin(), _output_availability.end(), 0);
}

void SB_QPS_HalfHalf_MI::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_HalfHalf_MI::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
    --_cf_packets_counter[s][d];
  }
}

int SB_QPS_HalfHalf_MI::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_HalfHalf_MI::_qps(size_t frame_id){

  // maximum number of accepts for each time slots
  int max_accepts = (((frame_id + 1) * 2 > frame_size())  ? 2 : 1);

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }
  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
    if (max_accepts > 1) {
      // sort proposals based on VOQ lengths
      // actually we can optimize by move sorting inside the if check
      // and whenever if fails, we just find the largest element and
      // move it to the beginning of the vector!
      std::sort(out_accepts[out].begin(), out_accepts[out].end(), [=](const int in1, const int in2){
        return _cf_packets_counter[in1][out] > _cf_packets_counter[in2][out];
      });

      for (int i = 1; i < std::min(max_accepts, (int)out_accepts[out].size()) && _output_availability[out] > 0; ++i) {
        int in = out_accepts[out][i];
        if (_input_availability[in] == 0) continue; // no available ts
        // available only when both available
        auto mf = (_match_flag_in[in] | _match_flag_out[out]);
        for (int f = (int) (frame_id) - 1; f >= 0; --f) {
          if (!mf.test(f)) {
            _match_flag_in[in].set(f);
            _match_flag_out[out].set(f);
            -- _input_availability[in];
            -- _output_availability[out];
            assert(_schedules[f][in] == -1);
            _schedules[f][in] = out;
            vdep.emplace_back(in, out);
            break;// pay special attention (first fit)
          }
        }
      }
    }

    int in = out_accepts[out][0];
    _match_flag_in[in].set(frame_id);
    _match_flag_out[out].set(frame_id);
    _schedules[frame_id][in] = out;
    out_match[out] = in;
    vdep.emplace_back(in, out);
  }

  for (int in = 0;in < num_inputs();++ in) {
    if (_schedules[frame_id][in] == -1) {
      // not matched in this time slot
      _input_availability[in] ++;
    }
  }

  for (int out = 0;out < num_outputs();++ out){
    if (out_match[out] == -1){
      // not matched
      _output_availability[out] ++;
    }
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}
void SB_QPS_HalfHalf_MI::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);
  int max_iters = (((frame_id + 1) * 2 > frame_size()) ? 3 : 1);
  for(int i = 0;i < max_iters;++i) _qps(frame_id);
}

void SB_QPS_HalfHalf_MI::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_HalfHalf_MI::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_HalfHalf_MI::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_MI::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}


// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_ThreeThird_MI
SB_QPS_ThreeThird_MI::SB_QPS_ThreeThird_MI(std::string name, int num_inputs, int num_outputs, int frame_size,
                                           std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _output_availability(num_outputs, 0),
    _input_availability(num_inputs, 0){
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_ThreeThird_MI::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
  std::fill(_input_availability.begin(), _input_availability.end(), 0);
  std::fill(_output_availability.begin(), _output_availability.end(), 0);
}

void SB_QPS_ThreeThird_MI::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_ThreeThird_MI::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
    --_cf_packets_counter[s][d];
  }
}

int SB_QPS_ThreeThird_MI::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_ThreeThird_MI::_qps(size_t frame_id){

  // maximum number of accepts for each time slots
  int max_accepts = 1;
  if((frame_id + 1) * 1.5 > _frame_size) max_accepts = 4;
  else if ((frame_id + 1) * 2 > _frame_size) max_accepts = 2;

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }
  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
    if (max_accepts > 1) {
      // sort proposals based on VOQ lengths
      // actually we can optimize by move sorting inside the if check
      // and whenever if fails, we just find the largest element and
      // move it to the beginning of the vector!
      std::sort(out_accepts[out].begin(), out_accepts[out].end(), [=](const int in1, const int in2){
        return _cf_packets_counter[in1][out] > _cf_packets_counter[in2][out];
      });

      for (int i = 1; i < std::min(max_accepts, (int)out_accepts[out].size()) && _output_availability[out] > 0; ++i) {
        int in = out_accepts[out][i];
        if (_input_availability[in] == 0) continue; // no available ts
        // available only when both available
        auto mf = (_match_flag_in[in] | _match_flag_out[out]);
        for (int f = (int) (frame_id) - 1; f >= 0; --f) {
          if (!mf.test(f)) {
            _match_flag_in[in].set(f);
            _match_flag_out[out].set(f);
            -- _input_availability[in];
            -- _output_availability[out];
            assert(_schedules[f][in] == -1);
            _schedules[f][in] = out;
            vdep.emplace_back(in, out);
            break;// pay special attention (first fit)
          }
        }
      }
    }

    int in = out_accepts[out][0];
    _match_flag_in[in].set(frame_id);
    _match_flag_out[out].set(frame_id);
    _schedules[frame_id][in] = out;
    out_match[out] = in;
    vdep.emplace_back(in, out);
  }

  for (int in = 0;in < num_inputs();++ in) {
    if (_schedules[frame_id][in] == -1) {
      // not matched in this time slot
      _input_availability[in] ++;
    }
  }

  for (int out = 0;out < num_outputs();++ out){
    if (out_match[out] == -1){
      // not matched
      _output_availability[out] ++;
    }
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}
void SB_QPS_ThreeThird_MI::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);
  int max_iters = 1;

  if((frame_id + 1) * 1.5 > _frame_size) max_iters = 3;
  else if ((frame_id + 1) * 2 > _frame_size) max_iters = 2;

  for(int i = 0;i < max_iters;++i) _qps(frame_id);
}

void SB_QPS_ThreeThird_MI::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_ThreeThird_MI::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_ThreeThird_MI::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_ThreeThird_MI::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}


// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_HalfHalf_MA
SB_QPS_HalfHalf_MA::SB_QPS_HalfHalf_MA(std::string name, int num_inputs, int num_outputs, int frame_size,
                                       std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _output_availability(num_outputs, 0),
    _input_availability(num_inputs, 0){
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_MA::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
  std::fill(_input_availability.begin(), _input_availability.end(), 0);
  std::fill(_output_availability.begin(), _output_availability.end(), 0);
}

void SB_QPS_HalfHalf_MA::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_HalfHalf_MA::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    //assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
  }
}

int SB_QPS_HalfHalf_MA::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_HalfHalf_MA::_qps(size_t frame_id){

  // maximum number of accepts for each time slots
  int max_accepts = (((frame_id + 1) * 2 > frame_size())  ? 2 : 1);

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }
  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
//    if (max_accepts > 1) {
    // sort proposals based on VOQ lengths
    // actually we can optimize by move sorting inside the if check
    // and whenever if fails, we just find the largest element and
    // move it to the beginning of the vector!
    std::sort(out_accepts[out].begin(), out_accepts[out].end(), [=](const int in1, const int in2) {
      return _cf_packets_counter[in1][out] > _cf_packets_counter[in2][out];
    });

    for (int i = 0; i < std::min(max_accepts, (int) out_accepts[out].size()) && _output_availability[out] > 0; ++i) {
      int in = out_accepts[out][i];
      if (_input_availability[in] == 0 || _cf_packets_counter[in][out] == 0)
        continue; // no available ts or VOQ is empty
      // available only when both available
      auto mf = (_match_flag_in[in] | _match_flag_out[out]);
      for (auto f = (int) (frame_id); f >= 0; --f) {
        if (!mf.test(f)) {
          _match_flag_in[in].set(f);
          _match_flag_out[out].set(f);
          --_input_availability[in];
          --_output_availability[out];
          --_cf_packets_counter[in][out];
          assert(_schedules[f][in] == -1);
          _schedules[f][in] = out;
          if (f == frame_id) out_match[out] = in;
          vdep.emplace_back(in, out);
        }
      }
    }
//    }

//    int in = out_accepts[out][0];
//    _match_flag_in[in].set(frame_id);
//    _match_flag_out[out].set(frame_id);
//    _schedules[frame_id][in] = out;
//    -- _cf_packets_counter[in][out];
//    out_match[out] = in;
//    vdep.emplace_back(in, out);
  }

  for (int in = 0;in < num_inputs();++ in) {
    if (_schedules[frame_id][in] == -1) {
      // not matched in this time slot
      _input_availability[in] ++;
    }
  }

  for (int out = 0;out < num_outputs();++ out){
    if (out_match[out] == -1){
      // not matched
      _output_availability[out] ++;
    }
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}
void SB_QPS_HalfHalf_MA::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);
  int max_iters = 1; // (((frame_id + 1) * 2 > frame_size()) ? 3 : 1);
  for(int i = 0;i < max_iters;++i) _qps(frame_id);
}

void SB_QPS_HalfHalf_MA::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_HalfHalf_MA::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_HalfHalf_MA::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_MA::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}

// ////////////////////////////////////////////////////////////////////////////
//        Implementation of class SB_QPS_HalfHalf_MA_MI
SB_QPS_HalfHalf_MA_MI::SB_QPS_HalfHalf_MA_MI(std::string name, int num_inputs, int num_outputs, int frame_size,
                                             std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _seed(seed),
    _eng(seed),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _output_availability(num_outputs, 0),
    _input_availability(num_inputs, 0){
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  _cf_rel_time = 0;
  // generate initial schedulers (used for the first frame)
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_MA_MI::bitmap_reset() {
  for (auto &mf : _match_flag_in) mf.reset();
  for (auto &mf : _match_flag_out) mf.reset();
  std::fill(_input_availability.begin(), _input_availability.end(), 0);
  std::fill(_output_availability.begin(), _output_availability.end(), 0);
}

void SB_QPS_HalfHalf_MA_MI::handle_arrivals(const IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++_cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS_HalfHalf_MA_MI::handle_departures(const std::vector<std::pair<int, int>> &dep_pre) {
  for (const auto &sd : dep_pre) {
    auto s = sd.first;
    auto d = sd.second;
    //assert(_cf_packets_counter[s][d] > 0);
    BST::update<int>(_bst[s], d + _left_start, -1);
  }
}

int SB_QPS_HalfHalf_MA_MI::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef DEBUG
  std::cerr << "random : " << r << "\n";
  std::cerr << "in : " << source << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
  std::cerr << "VOQ[i][j] : " << _cf_packets_counter[source][out] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

void SB_QPS_HalfHalf_MA_MI::_qps(size_t frame_id){

  // maximum number of accepts for each time slots
  int max_accepts = (((frame_id + 1) * 2 > frame_size())  ? 2 : 1);

  //std::vector<std::array<int, 2> > out_accepts(num_outputs());
  //for (auto &oac: out_accepts) oac.fill(-1);
  std::vector<std::vector<int> > out_accepts(num_outputs());

  // shuffle inputs
  std::vector<int> inputs(_num_inputs, 0);
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if (queue_length(in) == 0) continue;// no packets
    auto out = sampling(in);// sampling an output for this input
    assert(_cf_packets_counter[in][out] > 0);

    if (max_accepts > 1 ) {
      out_accepts[out].push_back(in);
    } else {// normal QPS
      if(out_accepts[out].empty()) out_accepts[out].push_back(in);
      else if (_cf_packets_counter[out_accepts[out].front()][out] < _cf_packets_counter[in][out])
        out_accepts[out][0] = in;
    }
  }

  std::vector<std::pair<int, int>> vdep;// virtual departures
  std::vector<int> out_match(num_outputs(), -1);
  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    if (out_accepts[out].empty()) continue;
//    if (max_accepts > 1) {
      // sort proposals based on VOQ lengths
      // actually we can optimize by move sorting inside the if check
      // and whenever if fails, we just find the largest element and
      // move it to the beginning of the vector!
      std::sort(out_accepts[out].begin(), out_accepts[out].end(), [=](const int in1, const int in2){
        return _cf_packets_counter[in1][out] > _cf_packets_counter[in2][out];
      });

      for (int i = 0; i < std::min(max_accepts, (int)out_accepts[out].size()) && _output_availability[out] > 0; ++i) {
        int in = out_accepts[out][i];
        if (_input_availability[in] == 0 || _cf_packets_counter[in][out] == 0) continue; // no available ts or VOQ is empty
        // available only when both available
        auto mf = (_match_flag_in[in] | _match_flag_out[out]);
        for (int f = (int) (frame_id) ; f >= 0; --f) {
          if (!mf.test(f)) {
            _match_flag_in[in].set(f);
            _match_flag_out[out].set(f);
            -- _input_availability[in];
            -- _output_availability[out];
            -- _cf_packets_counter[in][out];
            assert(_schedules[f][in] == -1);
            _schedules[f][in] = out;
            if(f == frame_id) out_match[out] = in;
            vdep.emplace_back(in, out);
          }
        }
      }
//    }

//    int in = out_accepts[out][0];
//    _match_flag_in[in].set(frame_id);
//    _match_flag_out[out].set(frame_id);
//    _schedules[frame_id][in] = out;
//    -- _cf_packets_counter[in][out];
//    out_match[out] = in;
//    vdep.emplace_back(in, out);
  }

  for (int in = 0;in < num_inputs();++ in) {
    if (_schedules[frame_id][in] == -1) {
      // not matched in this time slot
      _input_availability[in] ++;
    }
  }

  for (int out = 0;out < num_outputs();++ out){
    if (out_match[out] == -1){
      // not matched
      _output_availability[out] ++;
    }
  }

#ifdef DEBUG
  std::cerr << _cf_packets_counter << "\n";
  std::cerr << vdep << "\n\n";
#endif
  handle_departures(vdep);
}
void SB_QPS_HalfHalf_MA_MI::qps(const saber::IQSwitch *sw, size_t frame_id) {
  assert(_frame_size_fixed);

  // handle arrivals
  handle_arrivals(sw);
  int max_iters = (((frame_id + 1) * 2 > frame_size()) ? 3 : 1);
  for(int i = 0;i < max_iters;++i) _qps(frame_id);
}

void SB_QPS_HalfHalf_MA_MI::init(const IQSwitch *sw) {
  // reserved
}

void SB_QPS_HalfHalf_MA_MI::schedule(const saber::IQSwitch *sw) {
  auto frame_id = (_cf_rel_time % _frame_size);
  // copy out scheduler for the last frame
  std::copy(_schedules[frame_id].begin(), _schedules[frame_id].end(), _in_match.begin());
  std::fill(_schedules[frame_id].begin(), _schedules[frame_id].end(), -1);

  qps(sw, frame_id);

  // reset bit map, if it is the last time slot of a frame
  if (frame_id == _frame_size - 1) bitmap_reset();

  ++_cf_rel_time;
}

void SB_QPS_HalfHalf_MA_MI::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for (auto &counter : _cf_packets_counter)
    std::fill(counter.begin(), counter.end(), 0);
  _cf_rel_time = 0;
  for (auto &sched: _schedules) {
    std::fill(sched.begin(), sched.end(), -1);
  }
}

void SB_QPS_HalfHalf_MA_MI::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os << "seed             : " << _seed
     << "\nbst              : " << _bst
     << "\n";
}
} // namespace saber