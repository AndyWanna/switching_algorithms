//

#include "edge_coloring_batch_scheduler.h"
#include <switch/iq_switch.h>

namespace saber {
// Implementation of class RandomizedEdgeColoringCAggarwal
// /////////////////////////////////////////////////////////////////////////////////////////////////
RandomizedEdgeColoringCAggarwal::RandomizedEdgeColoringCAggarwal(std::string name,
                                                                 int num_inputs,
                                                                 int num_outputs,
                                                                 int frame_size,
                                                                 std::mt19937::result_type seed) :
    BatchScheduler(name, num_inputs, num_outputs, frame_size, true),
    _multigraph_adj(num_inputs),
    _seed(seed), _eng(seed) {

}
void RandomizedEdgeColoringCAggarwal::reset() {
  BatchScheduler::reset();
  for (auto& adj : _multigraph_adj) adj.clear();
}
void RandomizedEdgeColoringCAggarwal::display(std::ostream &os) const {
  BatchScheduler::display(os);
  os << "-------------------------------------------------------------\n";
  os << "seed : " << _seed
  << "\n";
}
void RandomizedEdgeColoringCAggarwal::handle_arrivals(const saber::IQSwitch *sw) {
  auto arrivals = sw->get_arrivals();

  for (const auto &sd : arrivals) {
    if (sd.first == -1)
      break;
    else {
      _multigraph_adj[sd.first].push_back(sd.second);
    }
  }
}

void RandomizedEdgeColoringCAggarwal::schedule(const IQSwitch *sw) {
  ++ _cf_rel_time ;
  if ( !_schedules_pre.empty() ) {//// not the first frame
    assert( _pf_rel_time < _schedules_pre.size() );
    for (size_t i = 0; i < _num_inputs; ++i) {
      _in_match[i] = _schedules_pre[_pf_rel_time][i];
      _schedules_pre[_pf_rel_time][i] = -1;
    }
    _pf_rel_time = _pf_rel_time + 1;
  }
  handle_arrivals(sw);
  if ( _cf_rel_time < _frame_size ) {
    return; // do nothing
  }
  size_t max_deg = 0;
  //const auto& q_matrix = sw->get_queue_matrix();
  for ( size_t i = 0;i < num_inputs();++ i){
    max_deg = std::max(max_deg, _multigraph_adj[i].size());
  }

  std::vector<size_t> perm(max_deg, 0);
  for ( size_t k = 0;k < max_deg;++ k) perm[k] = k;

  std::vector<std::bitset<MAX_FRAME_SIZE> > free_colors_in(_num_inputs);
  std::vector<std::bitset<MAX_FRAME_SIZE> > free_colors_out(_num_inputs);

  std::vector<std::vector<int> > next_free_color(num_inputs(), std::vector<int>(num_outputs(), _frame_size));
  std::shuffle(perm.begin(), perm.end(), _eng);
  size_t frame_size_next = _frame_size;
  for ( const auto& s : perm ) {
    for ( size_t i = 0;i < _num_inputs;++ i ) {
      if ( s >= _multigraph_adj[i].size() ) continue;
      //// Get the s^th edge of input i
      auto j = _multigraph_adj[i][s];

      std::vector<int> free_colors;
      for (int c = 0; c < _frame_size;++ c) {
        if ( !free_colors_in[i][c] && !free_colors_out[j][c] ) {
          free_colors.push_back(c);
        }
      }

      if ( free_colors.empty() ) {//// greedy coloring with extra free colors
        int gc = next_free_color[i][j];
        for (;gc < MAX_FRAME_SIZE;++ gc) {
          if ( !free_colors_in[i][gc] && !free_colors_out[j][gc] ) {
            free_colors_in[i].set(gc);
            free_colors_out[j].set(gc);
            while ( frame_size_next <= gc ) {
              _schedules.emplace_back(num_inputs(), -1);
              ++ frame_size_next;
            }
            _schedules[gc][i] = j;
          }
        }
        assert( gc < MAX_FRAME_SIZE );
        next_free_color[i][j] = gc + 1;
      } else {//// normal coloring
        std::uniform_int_distribution<> dis(0, free_colors.size() - 1);
        auto c = dis(_eng);
        free_colors_in[i].set(c);
        free_colors_out[j].set(c);
        _schedules[c][i] = j;
      }
    }
  }

  _cf_rel_time = 0;
  _frame_size = frame_size_next;
  assert( _pf_rel_time == _schedules_pre.size() );
  _schedules_pre.resize(_frame_size);

  for ( size_t k = 0;k < _frame_size;++ k){
    _schedules_pre[k].assign(_schedules[k].begin(), _schedules[k].end());
    std::fill(_schedules[k].begin(), _schedules[k].end(), - 1);
  }
  _pf_rel_time = 0;
}
void RandomizedEdgeColoringCAggarwal::init(const IQSwitch *sw) {
  // TODO
}
} // namespace saber