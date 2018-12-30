
#include "bernoulli_injection_model.h"


namespace saber{
BernoulliInjectionModel::BernoulliInjectionModel(int num_nodes, double rate,
                                                 std::mt19937::result_type seed) :
    RandomInjectionModel(num_nodes, rate, seed,"Bernoulli")
{ }

InjectionState BernoulliInjectionModel::inject(int source) {
  if (source < 0 || source >= _num_nodes) {
    std::cerr << "Argument source should be an integer within the range of [0, " << _num_nodes << ").\n";
    return InjectionState::OFF;
  }
  return (_distribution(_eng) < _rate ? InjectionState::ON : InjectionState::OFF);
}

void BernoulliInjectionModel::reset() {
  _distribution.reset();
}
} // namespace saber
