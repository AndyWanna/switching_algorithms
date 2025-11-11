// miscellaneous.h header file
// The schedulers implemented in this file are some straightforward scheduling algorithms, however never
// explicitly mentioned in literature, or only mentioned but not implemented.
//
// Current it contains:
//         -  sRR :  (simple) Round Robin
//         -  sLQF: (single-iteration) Longest Virtual Output Queue First
//         -  iPOC: (ierative) Power of (Two) Choices
#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include "scheduler.h"

namespace saber {
// Class for round robin scheduler
// /////////////////////////////////////////////////////////////////////////////////////////////
class RoundRobin : public Scheduler {
  friend class SchedulerFactory;
 protected:
  std::vector<int> _initial_match;
  RoundRobin(std::string name,
             int num_inputs,
             int num_outputs,
             std::vector<int> &initial_match);
 public:
  void reset() override { std::copy(_initial_match.begin(), _initial_match.end(), _in_match.begin());}
  void schedule(const IQSwitch *sw) override ;
  void init(const IQSwitch *sw) override { /* do nothing */ }
  void display(std::ostream &os) const override ;
}; // class RoundRobin

// Class for sLQF
// Note that sLQF inherits RandomizedScheduler because this implementation breaks tie randomly.
// /////////////////////////////////////////////////////////////////////////////////////////////
class sLQF : public RandomizedScheduler {
  friend class SchedulerFactory;
 protected:
  sLQF(std::string name,
       int num_inputs,
       int num_outputs,
       std::mt19937::result_type seed= static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count())) ;
 public:
  void reset() override {
  }
  void schedule(const IQSwitch *sw) override ;
  void init(const IQSwitch *sw) override ;
  void display(std::ostream &os) const override { RandomizedScheduler::display(os); }
}; // class sLQF

// Class for Power of Choices
// Here we only implement power of two choices.
// //////////////////////////////////////////////////////////////////////////////////////////////
class iPOC : public RandomizedScheduler {
  friend class SchedulerFactory;
 protected:
  int _num_iters;
  std::vector<std::vector<int> > _non_empty_voqs;

  iPOC(std::string name,
             int num_inputs,
             int num_outputs,
             int num_iters,
       std::mt19937::result_type seed= static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count()));
 public:
  void reset() override { for(auto& _voq: _non_empty_voqs) _voq.clear(); };
  void schedule(const IQSwitch *sw) override ;
  void init(const IQSwitch *sw) override ;
  void display(std::ostream &os) const override ;
};
} // namespace saber

#endif // MISCELLANEOUS_H
