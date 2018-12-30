// log_diagonal_traffic_pattern.h header file

#ifndef LOG_DIAGONAL_TRAFFIC_PATTERN_H
#define LOG_DIAGONAL_TRAFFIC_PATTERN_H

#include "traffic_pattern.h"

namespace saber {
// class for Log Diagonal Traffic Pattern
// //////////////////////////////////////////////////////////////////////////////
class LogDiagonalTrafficPattern : public RandomTrafficPattern {
  friend class TrafficPatternFactory;
 protected:
  LogDiagonalTrafficPattern(int num_nodes, std::mt19937::result_type seed) ;

 public:
  int destination(int source) override ;
  void reset() override ;
  void display(std::ostream &os) const override ;
  std::ostream& operator<<(std::ostream& os) const override;

  ~LogDiagonalTrafficPattern() override = default;

 private:
  double _diagonal_prob;
  // change from 16 to 30 @12-30-2018
  const int _exact_max_const{30};// The maximum num of nodes to use exact
  std::uniform_real_distribution<double> _distribution;
  std::vector<double> _constant_prob;
};
}

#endif // LOG_DIAGONAL_TRAFFIC_PATTERN_H
