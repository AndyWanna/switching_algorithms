// max_weight_scheduler.h header file

#ifndef MAX_WEIGHT_SCHEDULER_H
#define MAX_WEIGHT_SCHEDULER_H

#include "scheduler.h"
#include <lemon/list_graph.h>
#include <lemon/matching.h>

namespace saber {
using lemon::INVALID;
// Class for Maximum Weight Matching based Scheduler
// Note this class is simply a wrapper for lemon::MaxWeightedMatching<Graph, WeightMap>.
// If you are interested in which implementation of Maximum Weight Matching algorithm
// is used, please refer to <a HREF="http://lemon.cs.elte.hu/pub/doc/1.2.3/a00219.html">
// MaxWeightedMatching< GR, WM > Class Template Reference</a>.
// ////////////////////////////////////////////////////////////////////////////
class MaxWeightScheduler : public Scheduler {
  friend class SchedulerFactory;
 protected:
  typedef lemon::ListGraph Graph;
  typedef Graph::Node Node;
  typedef Graph::Edge Edge;
  typedef Graph::EdgeMap<int> WeightMap; // map each edge to a double number
  typedef lemon::MaxWeightedMatching<Graph, WeightMap> MaxWeightedMatching;

  Graph _g;// graph
  WeightMap _weight;// weight
  std::vector<Node> _left;// set of X
  std::vector<Node> _right;// set of Y
  std::vector<std::vector<int> > _edges;//
  bool _graph_initialized{false};

  MaxWeightScheduler(std::string name,
                     int num_inputs,
                     int num_outputs,
                     bool out_match_enabled);

  // Initialization of MaxWeightScheduler
  void initialize() ;
  // Update in match (_i.e._, the permutation mapping from input to output) according to MWM
  void update_in_match(const MaxWeightedMatching &mwm) ;
  // Update out match (_i.e._, the permutation mapping fron output to input)
  void update_out_match(const MaxWeightedMatching &mwm) ;
  // Reset the weights of all the edges
  void reset_weight() ;
  // Update the weight of a single edge from source to destination
  void update(int source, int destination, int new_weight) ;
  // Update edge weights according to arrivals
  void handle_arrivals(const IQSwitch *sw);
  // Update edge weights according to the resulting schedule
  void handle_departures(const IQSwitch *sw);
 public:
  ~MaxWeightScheduler() override = default;
  // Do the schedule (handle arrivals => calculate new schedule => handle departure)
  void schedule(const IQSwitch *sw) override;
  //
  void init(const IQSwitch *sw) override;
  // Reset the states of the scheduler
  void reset() override ;
};
} // namespace saber

#endif // MAX_WEIGHT_SCHEDULER_H
