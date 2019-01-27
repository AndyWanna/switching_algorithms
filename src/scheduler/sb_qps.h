// sb_qps.h header file
// Small-Batch (Maybe based on) Queue-Proportional Sampling
//

#ifndef SB_QPS_H
#define SB_QPS_H

#include "batch_scheduler.h"

namespace saber {
/** @brief Class for Half & Half SB_QPS
 *
 *  In the first half of a batch, just doing QPS-1 for each time slot,
 *  whereas in the second half, doing QPS-1 but allowing using holes left
 *  before to accept inputs' proposals.
 *
 */
class SB_QPS_HalfHalf_Oblivious : public BatchScheduler {
  friend class SchedulerFactory;
 protected:
  using bst_t = std::vector<int>;
  using bitmap_t = std::bitset<128>;
  std::mt19937::result_type _seed;
  std::mt19937 _eng{std::random_device{}()};

  int _left_start{-1};
  std::vector<bst_t> _bst;

  // bitmaps for each input & output
  std::vector<bitmap_t>  _match_flag_in;
  std::vector<bitmap_t>  _match_flag_out;

  // counter of packets
  std::vector<std::vector<int> > _cf_packets_counter;

  SB_QPS_HalfHalf_Oblivious(std::string name, int num_inputs, int num_outputs, int frame_size,
                  std::mt19937::result_type seed) ;
  void bitmap_reset() ;
  void handle_arrivals(const IQSwitch *sw);
  void handle_departures(const std::vector<std::pair<int, int>>& dep_pre);
  int sampling(int source);
  int queue_length(int source) {
    assert (source >= 0 && source < _num_inputs);
    return _bst[source][1];
  }
  void qps(const IQSwitch *sw, size_t current_ts);
 public:
  ~SB_QPS_HalfHalf_Oblivious() override = default;
  void schedule(const IQSwitch *sw) override;
  void init(const IQSwitch *sw) override;
  void reset() override ;
  void display(std::ostream &os) const override ;
  //// reserved
  void dump_stats(std::ostream &os) override {}
};
} // namespace saber

#endif // SB_QPS_H
