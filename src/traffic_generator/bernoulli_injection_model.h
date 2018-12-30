// bernoulli_injection_model.h header file
#ifndef INJECTION_MODEL_H
#define INJECTION_MODEL_H

#include "injection_model.h"

namespace saber {
// Class for Bernoulli Injection
// /////////////////////////////////////////////////////////////////////////////////////////////////
class BernoulliInjectionModel : public RandomInjectionModel {
  friend class InjectionModelFactory;
 protected:
  std::uniform_real_distribution<double> _distribution{0, 1.0};
  BernoulliInjectionModel(int num_nodes, double rate, std::mt19937::result_type seed);

 public:
  ~BernoulliInjectionModel() override = default;
  InjectionState inject(int source) override;
  void reset() override ;
}; // BernoulliInjectionModel
} // namespace saber
#endif // BERNOULLI_INJECTION_MODEL_H
