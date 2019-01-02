// maximal_scheduler.h header file

#ifndef MAXIMAL_SCHEDULER_H
#define MAXIMAL_SCHEDULER_H

#include "scheduler.h"

namespace saber {
// Class for randomized maximal matching scheduler
// ////////////////////////////////////////////////////////////////////////////////////////////
class RandomizedMaximalScheduler : public RandomizedScheduler {
  friend class SchedulerFactory;
 protected:
  using Edge=std::pair<int, int>;
  std::vector<Edge> _edges{};
  int _edge_num{0};
  std::vector<std::vector<int> > _index;//

  RandomizedMaximalScheduler(std::string name, int num_inputs, int num_outputs,
                             std::mt19937::result_type seed);
  /** @brief Insert an edge
   *
   * Note that this function only inserts an edge when the edge between
   * @source and @destination does not exist. If it exists, this function
   * will do nothing.
   *
   * @param source          Integer Identity of source
   * @param destination     Integer Identity of destination
   */
  void insert(int source, int destination) ;

  /** @brief Erase an edge
   *
   * This function first checks whether the edge between @source and @destination
   * exists. If it exists, then we remove the edge. Otherwise, it does nothing.
   *
   * @param source          Integer Identity of source
   * @param destination     Integer Identity of destination
   */
  void erase(int source, int destination) ;

  void handle_arrivals(const IQSwitch *sw);
  void handle_departures(const IQSwitch *sw);
 public:
  ~RandomizedMaximalScheduler() override = default;
  void schedule(const IQSwitch *sw) override;
  void init(const IQSwitch *sw) override;
  void reset() override ;
}; // RandomizedMaximalScheduler

// Class for <a href="https://dl.acm.org/citation.cfm?id=310896">iSLIP</a>
// ///////////////////////////////////////////////////////////////////////////////////////////////////
class iSLIP : public Scheduler {
  friend class SchedulerFactory;
 protected:
  int _iterations;
  std::vector<int> _grant_pointers;
  std::vector<int> _accept_pointer;

  std::vector<std::vector<bool> > _queue_status;

  iSLIP(const std::string &name,
        int num_inputs,
        int num_outputs,
        int iterations,
        const std::vector<int> &grant_pointers,
        const std::vector<int> &accept_pointer);


  void handle_arrivals(const IQSwitch *sw);
  void handle_departures(const IQSwitch *sw);
 public:
  ~iSLIP() override = default;
  void reset() override ;
  void display(std::ostream &os) const override;
  void schedule(const IQSwitch *sw) override;
  void init(const IQSwitch *sw) override;
};
} // namespace saber

#endif // MAXIMAL_SCHEDULER_H
