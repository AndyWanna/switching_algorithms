// sb_qps.h header file
// Small-Batch (Maybe based on) Queue-Proportional Sampling
//

#ifndef SB_QPS_H
#define SB_QPS_H

#include "batch_scheduler.h"

namespace saber {
// Class for SB-QPS
class SB_QPS : public BatchScheduler {
  friend class SchedulerFactory;
 protected:
  using bst_t = std::vector<int>;
  using bitmap_t = std::bitset<FRAME_SIZE_BLOCK>;
  using frame_id = std::pair<uint16_t /* which frame block */, uint16_t /* index inside frame block */>;

  std::mt19937::result_type _seed;
  std::mt19937 _eng{std::random_device{}()};
  // whether or not to allow retry previous time slots within the same frame
  bool _allow_retry_previous{false};
  // accept policy
  std::string _accept_policy{"longest_first"};

  int _left_start{-1};
  std::vector<bst_t> _bst;

  // bitmaps for each input & output
  std::vector<std::vector<bitmap_t> > _match_flag_in;
  std::vector<std::vector<bitmap_t> > _match_flag_out;

  // counter of packets
  std::vector<std::vector<int> > _cf_packets_counter;
  // packets to be served
  // std::vector<std::pair<int /* source */, int /* destination */> > _remaining_packets;
  // next try color for each edge
  std::vector<std::vector<frame_id> > _next_try_color;

  SB_QPS(std::string name, int num_inputs, int num_outputs, int frame_size, bool frame_size_fixed,
           std::mt19937::result_type seed, bool allow_retry_previous, std::string accept_policy) ;
  void handle_arrivals(const IQSwitch *sw);
  void qps(const IQSwitch *sw, size_t current_ts);
  void qps_adaptive_frame(const IQSwitch *sw, size_t current_ts);
  void assign_previous(int s, int d, size_t current_ts);
  void post_optimization();
  void post_optimization_adaptive_frame();
  void handle_departures(const std::vector<int>& in_match);
  inline int sampling(int source);
  inline int queue_length(int source);
  void bitmap_reset() ;
 public:
  ~SB_QPS() override = default;
  void schedule(const IQSwitch *sw) override;
  void init(const IQSwitch *sw) override;
  void reset() override ;
  void display(std::ostream &os) const override ;
  //// reserved
  void dump_stats(std::ostream &os) override {}
};
} // namespace saber

#endif // SB_QPS_H
