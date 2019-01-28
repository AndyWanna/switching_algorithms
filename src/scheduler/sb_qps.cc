
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
  // generate random schedulers (used for the first frame)
  for (auto &sched: _schedules) {
//    for (size_t in = 0; in < _num_inputs; ++in) sched[in] = in;
//    std::shuffle(sched.begin(), sched.end(), _eng);
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
//  for (int s = 0; s < _num_inputs; ++s) {
//    auto d = _in_match[s];
//    if (d != -1) {
//      BST::update<int>(_bst[s], d + _left_start, -1);
//      --_cf_packets_counter[s][d];
//    }
//  }
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

} // namespace saber