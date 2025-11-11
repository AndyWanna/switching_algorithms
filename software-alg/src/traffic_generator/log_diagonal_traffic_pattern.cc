
#include "log_diagonal_traffic_pattern.h"

namespace saber {
// Implementation for class LogDiagonalTrafficPattern
// /////////////////////////////////////////////////////////////////////////////////////////////////////
LogDiagonalTrafficPattern::LogDiagonalTrafficPattern(int num_nodes, std::mt19937::result_type seed) :
    RandomTrafficPattern(num_nodes, seed, "log-diagonal"),
    _diagonal_prob(0.5), _distribution(0, 1.0) {
  if (num_nodes <= _exact_max_const) {
    // Use the exact value instead of the approximate one (i.e., 0.5)
    _diagonal_prob = (1 << (num_nodes - 1)) * 1.0 / ((1 << num_nodes) - 1);
  }
  double prob = _diagonal_prob;
  double cum_prob = _diagonal_prob;
  for (size_t i = 0; i < std::min(num_nodes, _exact_max_const - 1); ++i) {
    _constant_prob.push_back(cum_prob);
    prob *= 0.5;
    cum_prob += prob;
  }
  _constant_prob.push_back(1.0);
}

void LogDiagonalTrafficPattern::display(std::ostream &os) const {
  RandomTrafficPattern::display(os);
  os << "constants  : " << _constant_prob
     << "\n";
}
std::ostream &LogDiagonalTrafficPattern::operator<<(std::ostream &os) const {
  RandomTrafficPattern::operator<<(os);
  os << "\nconstants  : " << _constant_prob;
  return os;
}
int LogDiagonalTrafficPattern::destination(int source) {
    if (source < 0 || source >= _num_nodes) {
      std::cerr << "source should be an integer within [0, " << _num_nodes << ")\n";
      return -1;
    }
    auto c = _distribution(_eng);
    int cnt = 0;
    for (const auto &p : _constant_prob) {
      if (c < p) {
        return ((source + cnt) % _num_nodes);
      }
      ++cnt;
    }

    assert(false);// This line should not be reached!
    return -1;
}
void LogDiagonalTrafficPattern::reset() {
  RandomTrafficPattern::reset();
  _distribution.reset();
}
} // namespace saber
