
#include "quasi_diagonal_traffic_pattern.h"

namespace saber {
// Implementation of class QuasiDiagonalTrafficPattern
// ///////////////////////////////////////////////////////////////////////////////////////////////////////
QuasiDiagonalTrafficPattern::QuasiDiagonalTrafficPattern(int num_nodes, std::mt19937::result_type seed) :
    RandomTrafficPattern(num_nodes, seed, "quasi-diagonal"),
    _distribution(0, num_nodes) {
}

int QuasiDiagonalTrafficPattern::destination(int source) {
  if (source < 0 || source >= _num_nodes) {
    std::cerr << "source should be an integer within [0, " << _num_nodes << ")\n";
    return -1;
  }
  auto c = _distribution(_eng);
  if (c < (0.5 * ((double) _num_nodes))) {
    return source;
  } else {
    auto h = 0.5;
    const double q = 1.0 / ((_num_nodes - 1.0) * 2.0);
    for (int j = 1; j < _num_nodes; j++) {
      h = h + q;
      if (c < (h * ((double) _num_nodes))) {
        return (source + j) % _num_nodes;
      }
    }
  }
  assert(false);// this line should not be reached
  return -1;
}
void QuasiDiagonalTrafficPattern::reset() {
  RandomTrafficPattern::reset();
  _distribution.reset();
}
void QuasiDiagonalTrafficPattern::display(std::ostream &os) const {
  RandomTrafficPattern::display(os);
}
std::ostream &QuasiDiagonalTrafficPattern::operator<<(std::ostream &os) const {
  return RandomTrafficPattern::operator<<(os);
}
} // namespace saber
