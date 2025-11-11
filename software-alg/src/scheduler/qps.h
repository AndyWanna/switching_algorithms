//   qps.h header file

#ifndef QPS_H
#define QPS_H

#include "scheduler.h"

namespace saber {
// Class for <a href="">Queue proportional Sampling</a>
// /////////////////////////////////////////////////////////////////////
class QPS : public RandomizedScheduler {
  friend class SchedulerFactory;
 protected:
  using bst_t = std::vector<int>;
  int _iterations{1};

  std::string _accept_policy{"longest_first"};
  bool _without_replacement{false};
  int _left_start{-1};

  std::vector<bst_t> _bst;

  QPS(std::string name,
      int num_inputs,
      int num_outputs,
      std::mt19937::result_type seed,
      int iterations,
      std::string accept_policy,
      bool without_replacement);

  void handle_arrivals(const IQSwitch *sw);
  void handle_departures(const IQSwitch *sw);
  inline int sampling(int source);
  inline int queue_length(int source);
  inline int remove_voq(int source, int destination);
  inline void restore_voq(const std::vector<std::pair<std::pair<int, int>, int> > &restore_pairs);
 public:

  ~QPS() override = default;
  void reset() override ;
  void schedule(const IQSwitch *sw) override;
  void init(const IQSwitch *sw) override;
  void display(std::ostream &os) const override ;
};
} // namespace saber

#endif // QPS_H
