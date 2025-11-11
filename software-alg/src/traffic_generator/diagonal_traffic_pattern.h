// diagonal_traffic_pattern.h header file

#ifndef DIAGONAL_TRAFFIC_PATTERN_H
#define DIAGONAL_TRAFFIC_PATTERN_H

#include "traffic_pattern.h"

namespace saber {
// class for diagonal traffic
// //////////////////////////////////////////////////////////////////////////////////
class DiagonalTrafficPattern : public RandomTrafficPattern {
  friend class TrafficPatternFactory;
 protected:
  DiagonalTrafficPattern(int num_nodes, std::mt19937::result_type seed) ;

 public:
  int destination(int source) override ;
  void reset() override ;
  void display(std::ostream& os) const override;
  std::ostream& operator<<(std::ostream& os) const override;
  ~DiagonalTrafficPattern() override = default;

 private:
  std::uniform_real_distribution<double> _distribution;
  const double _two_thirds{2.0 / 3.0};
};
}

#endif // DIAGONAL_TRAFFIC_PATTERN_H
