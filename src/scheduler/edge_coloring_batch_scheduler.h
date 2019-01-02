// edge_color_batch_shceduler.h header file
// This file contains a few batch schedulers based on some edge coloring algorithms.
//
// More precisely, it contains:
//        RandomizedEdgeColoringCAggarwal - Simple randomized edge coloring based scheduler
//


#ifndef EDGE_COLORING_BATCH_SCHEDULER_H
#define EDGE_COLORING_BATCH_SCHEDULER_H

#include "batch_scheduler.h"

namespace saber {
// Class for randomized edge coloring based scheduler
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
class RandomizedEdgeColoringCAggarwal : public BatchScheduler {
  friend class SchedulerFactory;
 protected:
  std::vector<std::vector<int> > _multigraph_adj;
  std::mt19937::result_type _seed;
  std::mt19937 _eng{std::random_device{}()};
  RandomizedEdgeColoringCAggarwal(std::string name, int num_inputs, int num_outputs, int frame_size,
                                  std::mt19937::result_type seed);
  void handle_arrivals(const IQSwitch *sw);
 public:
  ~RandomizedEdgeColoringCAggarwal() override = default;
  void schedule(const IQSwitch *sw) override;
  void init(const IQSwitch *sw) override;
  void reset() override ;
  void display(std::ostream &os) const override ;
  //// reserved
  void dump_stats(std::ostream &os) override { }
}; // class RandomizedEdgeColoringCAggarwal

} // namespace saber

#endif // EDGE_COLORING_BATCH_SCHEDULER_H
