// uniform_traffic_pattern.h header file


#ifndef UNIFORM_TRAFFIC_PATTERN_H
#define UNIFORM_TRAFFIC_PATTERN_H

#include "traffic_pattern.h"
namespace saber{
// Uniform Traffic Pattern
// ///////////////////////////////////////////////////////////////////////////
class UniformTrafficPattern : public RandomTrafficPattern {
  friend class TrafficPatternFactory;
 protected:
  UniformTrafficPattern(int num_nodes, std::mt19937::result_type seed) ;
 public:
  /** @brief Destination API
   *
   *  This API determines which destination should the traffic goes to, if there is
   *  a unit of traffic injecting into source interface.
   *  In uniform traffic pattern, the destination is uniformly randomly selected from all
   *  the possible destinations for the source.
   *
   * @param source     Integer Source interface identity
   * @return           Integer Destination interface identity
   */
  int destination(int source) override ;
  void reset() override ;
  ~UniformTrafficPattern() override = default;
 private:
  std::uniform_int_distribution<int> _distribution;
};

} // namespace saber

#endif // UNIFORM_TRAFFIC_PATTERN_H
