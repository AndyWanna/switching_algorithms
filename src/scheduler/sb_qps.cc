
#include "sb_qps.h"
#include <switch/iq_switch.h>

namespace saber {
// Implementation of class SB_QPS
// //////////////////////////////////////////////////////////////////////////////////////////////////////////
SB_QPS::SB_QPS(std::string name,
               int num_inputs,
               int num_outputs,
               int frame_size,
               std::mt19937::result_type seed,
               bool allow_retry_previous,
               bool allow_adaptive_frame,
               std::string accept_policy) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size),
    _seed(seed), _eng(seed),
    _allow_retry_previous(allow_retry_previous),
    _allow_adaptive_frame(allow_adaptive_frame),
    _accept_policy(std::move(accept_policy)),
    _bst(num_inputs),
    _match_flag_in(num_inputs),
    _match_flag_out(num_outputs),
    _cf_packets_counter(num_inputs, std::vector<int>(num_outputs, 0)),
    _next_try_color(num_inputs, std::vector<frame_id>(num_outputs, {0, 0}))
{
  _left_start = BST::nearest_power_of_two(_num_outputs);
  for (size_t i = 0; i < num_inputs; ++i) _bst[i].resize(2 * _left_start, 0);
  size_t num_of_frame_blocks = ( _frame_size + FRAME_SIZE_BLOCK - 1 ) / FRAME_SIZE_BLOCK;
  for ( auto & mf : _match_flag_in ) {
    mf.resize(num_of_frame_blocks, bitmap_t());
  }
  for ( auto & mf : _match_flag_out ) {
    mf.resize(num_of_frame_blocks, bitmap_t());
  }
}
void SB_QPS::bitmap_reset() {
  size_t num_of_frame_blocks = ( _frame_size + FRAME_SIZE_BLOCK - 1 ) / FRAME_SIZE_BLOCK;
  for ( auto & mf : _match_flag_in ) {
    mf.resize(num_of_frame_blocks);
    for ( auto& bm : mf ) bm.reset();
  }
  for ( auto & mf : _match_flag_out ) {
    mf.resize(num_of_frame_blocks);
    for ( auto& bm : mf ) bm.reset();
  }
}
void SB_QPS::reset() {
  BatchScheduler::reset();
  bitmap_reset();
  for (size_t i = 0; i < _num_inputs; ++i)
    std::fill(_bst[i].begin(), _bst[i].end(), 0);
  for ( auto & counter : _cf_packets_counter )
    std::fill( counter.begin(), counter.end(), 0);
}
void SB_QPS::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "---------------------------------------------------------------------\n";
  os <<   "seed             : " << _seed
     << "\naccepting policy : " << _accept_policy
     << "\nbst              : " << _bst
     << "\n";
}
void SB_QPS::handle_arrivals(const saber::IQSwitch *sw) {
  assert (sw != nullptr);
  auto arrivals = sw->get_arrivals();
  for (const auto &sd : arrivals) {
    if (sd.first == -1) break;
    assert (sd.first >= 0 && sd.first < _num_inputs);
    assert(sd.second >= 0 && sd.second < _num_outputs);
    BST::update<int>(_bst[sd.first], sd.second + _left_start);
    ++ _cf_packets_counter[sd.first][sd.second];
  }
}

void SB_QPS::handle_departures(const std::vector<int>& in_match) {
  for (int s = 0; s < _num_inputs; ++s) {
    auto d = in_match[s];
    if (d != -1) {
      BST::update<int>(_bst[s], d + _left_start, -1);
      -- _cf_packets_counter[s][d];
    }
  }
}

int SB_QPS::sampling(int source) {
  assert (source >= 0 && source < _num_inputs);
  std::uniform_real_distribution<double> dist(0, _bst[source][1]);
  double r = dist(_eng);

  int out = BST::upper_bound<int>(_bst[source], r) - _left_start;

#ifdef _DEBUG_QPS_
  std::cerr << "random : " << r << "\n";
  std::cerr << "out : " << out << "\n";
  std::cerr << "bst : " << _bst[source] << "\n";
#endif

  assert (out >= 0 && out < _num_outputs);
  return out;
}

int SB_QPS::queue_length(int source) {
  assert (source >= 0 && source < _num_inputs);
  return _bst[source][1];
}

void SB_QPS::qps( const saber::IQSwitch *sw, size_t current_ts ) {
  handle_arrivals(sw);

  std::vector<int> in_match(num_inputs(), -1);
  std::vector<int> out_match(num_outputs(), -1);

  std::vector<int> requests(_num_inputs, -1);
  std::vector<int> last_check(_num_outputs, -1);

  std::vector<int> inputs(_num_inputs, 0);

  // shuffle inputs
  for (int i = 0; i < _num_inputs; ++i) inputs[i] = i;
  std::shuffle(inputs.begin(), inputs.end(), _eng);

  // Step 1: Proposing
  for (int i = 0; i < _num_inputs; ++i) {
    int in = inputs[i];
    if ( queue_length(in) == 0 || in_match[in] != -1 ) continue;// no packets or already matched

    auto out = sampling(in);// sampling an output for this input
    requests[in] = out;// record it
    if ( out_match[out] != -1 ) {// already matched
      /// actually this line should never be reached
      if ( _allow_retry_previous ) assign_previous( in, out, current_ts );
      continue;
    }

    // Note that last_check[out] records the input ports whose request is "accepted" by
    // the output
    if (last_check[out] != -1) {// requested before
      if (_accept_policy == "longest_first") {
        if (sw->get_queue_length(in, out) > sw->get_queue_length(last_check[out], out)) {
          if ( _allow_retry_previous ) assign_previous(last_check[out], out, current_ts);
          last_check[out] = in;
        } else {
          if ( _allow_retry_previous ) assign_previous(in, out, current_ts);
        }
      } else if (_accept_policy == "earliest_first" || _accept_policy == "random") {
        if ( _allow_retry_previous ) assign_previous(in, out, current_ts);
        // do nothing
      } else if (_accept_policy == "shortest_first") {
        if (sw->get_queue_length(in, out) < sw->get_queue_length(last_check[out], out)) {
          if ( _allow_retry_previous ) assign_previous(last_check[out], out, current_ts);
          last_check[out] = in;
        } else {
          if ( _allow_retry_previous ) assign_previous(in, out, current_ts);
        }
      }
    } else {// the first one
      last_check[out] = in;
    }

  }

#ifdef _DEBUG_QPS_
  std::cerr << "\trequests : " << requests << "\n";
    std::cerr << "\tlast_check : " << last_check << "\n";
#endif

  // Step 2: Accept
  for (int out = 0; out < _num_outputs; ++out) {
    int in = last_check[out];
    // in is eventually accepted by out and in in unmatched
    if (in != -1 && in_match[in] == -1) {
      out_match[out] = in;
      in_match[in] = out;
    }
  }

  //// copy newly calculated matching & update match flags
  auto frame_id = current_ts / FRAME_SIZE_BLOCK;
  auto inside_frame_id = current_ts % FRAME_SIZE_BLOCK;

  for ( size_t i = 0;i < num_inputs();++ i ) {
    auto j = in_match[i];
    _schedules[current_ts][i] = j;
    if ( j != -1 ) {
      _match_flag_in[i][frame_id].set(inside_frame_id);
      _match_flag_out[j][frame_id].set(inside_frame_id);
    }
  }
  handle_departures(in_match);
}

// try the best to serve all packets that we failed to in the QPS stage
void SB_QPS::post_optimization() {
  std::vector<std::pair<int, int> > remaining_packets;
  for ( size_t i = 0;i < num_inputs();++ i ) {
    for ( size_t j = 0;j < num_outputs();++ j ) {
      int pkt_cnt = _cf_packets_counter[i][j];
      while ( pkt_cnt > 0 ) {
        remaining_packets.emplace_back(i, j);
        --pkt_cnt;
      }
    }
  }

  int c;

  std::shuffle(remaining_packets.begin(), remaining_packets.end(), _eng);
  for ( auto &e : remaining_packets ) {//// try to color each edge
    auto i = e.first;
    auto j = e.second;
    auto nt_fid = _next_try_color[i][j].first;
    auto nt_color = _next_try_color[i][j].second;
    bool found = false;

    c = nt_color;
    int ts = nt_fid * FRAME_SIZE_BLOCK + c;
    for ( size_t fid = nt_fid; !found && fid < _match_flag_in[i].size(); ++ fid ) {
      auto &bm_in = _match_flag_in[i][fid];
      auto &bm_out = _match_flag_out[j][fid];

      for (; !found && c < FRAME_SIZE_BLOCK && (ts  < _frame_size); ++c, ++ ts) {
        if ( !bm_in[c] && !bm_out[c] ) {
          found = true;
          bm_in.set(c);
          bm_out.set(c);
          assert( _schedules[ts][i] == -1 );
          _schedules[ts][i] = j;
          -- _cf_packets_counter[i][j];
          BST::update<int>(_bst[i], j + _left_start, -1);
          _next_try_color[i][j] = ( c == FRAME_SIZE_BLOCK - 1 ? std::make_pair(fid + 1, 0) : std::make_pair(fid, c + 1));
        }
      }
      c = 0;
    }
  }

  for ( size_t i = 0;i < num_inputs();++ i )
    for ( size_t j = 0;j < num_outputs();++ j)
      _next_try_color[i][j] = {0, 0};
}

// reserved
void SB_QPS::qps_adaptive_frame(const IQSwitch *sw, size_t current_ts) {
  qps(sw, current_ts);
}

void SB_QPS::post_optimization_adaptive_frame() {

  std::vector<std::pair<int,int> > remaining_packets;
  for ( size_t i = 0;i < num_inputs();++ i ) {
    for ( size_t j = 0;j < num_outputs();++ j ) {
      auto pkt_cnt = _cf_packets_counter[i][j];
      while ( pkt_cnt > 0 ) {
        remaining_packets.emplace_back(i, j);
        --pkt_cnt;
      }
    }
  }
  int c;

  std::shuffle(remaining_packets.begin(), remaining_packets.end(), _eng);
  for ( auto &e : remaining_packets ) {//// try to color each edge
    auto i = e.first;
    auto j = e.second;
    auto nt_fid = _next_try_color[i][j].first;
    auto nt_color = _next_try_color[i][j].second;
    bool found = false;

    c = nt_color;
    auto ts = nt_fid * FRAME_SIZE_BLOCK + c;
    for ( size_t fid = nt_fid; !found ; ++ fid ) {
      if ( fid >= _match_flag_in[i].size() ) {
        _match_flag_in[i].resize(fid + 1);
      }
      if ( fid >= _match_flag_out[j].size() ) {
        _match_flag_out[j].resize(fid + 1);
      }
      auto &bm_in = _match_flag_in[i][fid];
      auto &bm_out = _match_flag_out[j][fid];

      for (; !found && c < FRAME_SIZE_BLOCK ; ++c, ++ts) {
        if ( !bm_in[c] && !bm_out[c] ) {
          found = true;
          bm_in.set(c);
          bm_out.set(c);

          if ( ts >= _schedules.size() ) {
            _schedules.resize(ts + 1, std::vector<int>(num_inputs(), -1));
          }
          _schedules[ts][i] = j;
          -- _cf_packets_counter[i][j];
          BST::update<int>(_bst[i], j + _left_start, -1);
          _next_try_color[i][j] = ( c == FRAME_SIZE_BLOCK - 1 ? std::make_pair(fid + 1, 0) : std::make_pair(fid, c + 1));
        }
      }
      c = 0;
    }
  }

  for ( size_t i = 0;i < num_inputs();++ i )
    for ( size_t j = 0;j < num_outputs();++ j)
      _next_try_color[i][j] = {0, 0};
}

void SB_QPS::assign_previous(int s, int d, size_t current_ts) {
  size_t fid = current_ts  / FRAME_SIZE_BLOCK;
  size_t c = ( current_ts  % FRAME_SIZE_BLOCK );

  if ( c == 0 ) { c = FRAME_SIZE_BLOCK - 1; -- fid; }
  else { -- c; }

  if ( fid < 0 ) return;

  auto nt_fid = _next_try_color[s][d].first;
  auto nt_color = _next_try_color[s][d].second;

  if ( nt_fid > fid || ( nt_fid == fid && nt_color > c ) ) return;

  bool found = false;

  auto c_ = nt_color;
  for ( size_t fid_ = nt_fid; !found && fid_ <= fid; ++ fid_ ) {
    auto &bm_in = _match_flag_in[s][fid_];
    auto &bm_out = _match_flag_out[d][fid_];

    auto max_color = ( fid_ == fid ? c : FRAME_SIZE_BLOCK - 1 );
    for (; !found && c_ <= max_color; ++ c_) {
      if ( !bm_in[c_] && !bm_out[c_] ) {
        found = true;

        bm_in.set(c_);
        bm_out.set(c_);
        _schedules[fid_ * FRAME_SIZE_BLOCK + c_][s] = d;
        -- _cf_packets_counter[s][d];
        BST::update<int>(_bst[s], d + _left_start, -1);

        if ( c_ == FRAME_SIZE_BLOCK - 1 ) {
          _next_try_color[s][d] = {fid_ + 1, 0};
        } else {
          _next_try_color[s][d] = {fid_, c_ + 1};
        }
      }
    }
    c_ = 0;
  }
}

void SB_QPS::schedule(const saber::IQSwitch *sw) {
  if ( !_schedules_pre.empty() ) {//// not the first frame
    assert( _pf_rel_time < _schedules_pre.size() );
    for ( size_t i = 0; i < _num_inputs; ++i ) {
      _in_match[i] = _schedules_pre[_pf_rel_time][i];
      _schedules_pre[_pf_rel_time][i] = -1;
    }
    //// please comment out me
    assert(saber::is_a_matching(_in_match));
    _pf_rel_time = _pf_rel_time + 1;
  }

  qps(sw, _cf_rel_time);
  ++ _cf_rel_time;
  if ( _cf_rel_time == _frame_size ) {
    if ( _allow_adaptive_frame ) post_optimization_adaptive_frame();
    else post_optimization();

    _cf_rel_time = 0;
    _frame_size = _schedules.size();
    assert(_pf_rel_time == _schedules_pre.size());
    _schedules_pre.resize(_frame_size);

    for ( size_t k = 0;k < _frame_size;++ k){
      _schedules_pre[k].assign(_schedules[k].begin(), _schedules[k].end());
      std::fill(_schedules[k].begin(), _schedules[k].end(), - 1);
    }
    _pf_rel_time = 0;
    bitmap_reset();
  }
}
void SB_QPS::init(const IQSwitch *sw) {
  // TODO
}

} // namespace saber