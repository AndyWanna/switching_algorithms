

#include "uniform_traffic_pattern.h"

namespace saber {
// Implementation for class UniformTrafficPattern
UniformTrafficPattern::UniformTrafficPattern(int num_nodes, std::mt19937::result_type seed) :
    RandomTrafficPattern(num_nodes, seed, "uniform"), _distribution(0, num_nodes - 1) {}

int UniformTrafficPattern::destination(int source) {
  if (source < 0 || source >= _num_nodes) {
    std::cerr << "source should be an integer within [0, " << _num_nodes << ")\n";
    return -1;
  }
  return _distribution(_eng);
}
void UniformTrafficPattern::reset() {
    _distribution.reset();
}

} // namespace saber
