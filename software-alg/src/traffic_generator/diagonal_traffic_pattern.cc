// diagonal_traffic_pattern.cc file

#include "diagonal_traffic_pattern.h"

namespace saber {
// Implementation of class DiagonalTrafficPattern
// /////////////////////////////////////////////////////////////////////////////////////////////
DiagonalTrafficPattern::DiagonalTrafficPattern(int num_nodes, std::mt19937::result_type seed) :
    RandomTrafficPattern(num_nodes, seed, "diagonal"),
    _distribution(0, 1.0) {

}
int DiagonalTrafficPattern::destination(int source) {
  if (source < 0 || source >= _num_nodes) {
    std::cerr << "source should be an integer within [0, " << _num_nodes << ")\n";
    return -1;
  }
  auto c = _distribution(_eng);
  if (c < _two_thirds)
    return source;
  return (source + 1) % _num_nodes;
}
void DiagonalTrafficPattern::reset() {
  RandomTrafficPattern::reset();
  _distribution.reset();
}
void DiagonalTrafficPattern::display(std::ostream &os) const {
  RandomTrafficPattern::display(os);
}
std::ostream &DiagonalTrafficPattern::operator<<(std::ostream &os) const {
  return RandomTrafficPattern::operator<<(os);
}

} // namespace saber
