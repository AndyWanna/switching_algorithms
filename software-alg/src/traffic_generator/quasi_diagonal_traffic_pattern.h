// quasi_diagonal_traffic_pattern.h header file

#ifndef QUASI_DIAGONAL_TRAFFIC_PATTERN_H
#define QUASI_DIAGONAL_TRAFFIC_PATTERN_H

#include "traffic_pattern.h"

namespace saber {
// class for quasi-diagonal traffic pattern
// ////////////////////////////////////////////////////////////////////////////////////
class QuasiDiagonalTrafficPattern : public RandomTrafficPattern {
  friend class TrafficPatternFactory;
 protected:
  QuasiDiagonalTrafficPattern(int num_nodes, std::mt19937::result_type seed) ;

 public:
  int destination(int source) override ;
  void reset() override ;
  void display(std::ostream& os) const override;
  std::ostream& operator<<(std::ostream& os) const override;
  ~QuasiDiagonalTrafficPattern() override = default;

 private:
  std::uniform_real_distribution<double> _distribution;
};
}

#endif // QUASI_DIAGONAL_TRAFFIC_PATTERN_H
