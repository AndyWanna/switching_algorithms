#include "onoff_injection_model.h"


namespace saber {
OnOffInjectionModel::OnOffInjectionModel(int num_nodes,
                                         double rate,
                                         std::mt19937::result_type seed,
                                         double alpha,
                                         double beta,
                                         double burst_size_avg,
                                         const std::vector<bool> &initial) :
    RandomInjectionModel(num_nodes, rate, seed, "OnOff"), _alpha(alpha), _beta(beta),
    _burst_size_avg(burst_size_avg),
    _initial(initial), _state(initial) {

  assert((alpha == -1 && beta == -1 && burst_size_avg > 1) ||
      (alpha == -1 && beta >= 0 && beta < 1.0 && burst_size_avg == -1) ||
      (alpha > 0 && alpha <= 1.0 && beta == -1 && burst_size_avg == -1) &&
          "Exact one argument of alpha, beta, or burst_size_avg should be set. ");


  if (alpha != -1) {
    _beta = (rate * alpha) / (1.0 - rate);
    _burst_size_avg = 1.0 / _alpha;
  } else if (beta != -1) {
    _alpha = (1.0 - rate) * beta / rate;
    _burst_size_avg = 1.0 / _alpha;
  } else {
    _alpha = 1.0 / burst_size_avg;
    _beta = (rate * _alpha) / (1.0 - rate);
  }

  // Fix the cases when alpha or beta gets larger than one!!
  assert(_alpha < 1.0 && "beta can not be larger than 1.0!");
  assert(_beta < 1.0 && "beta can not be larger than 1.0!");
}

void OnOffInjectionModel::reset() {
    RandomInjectionModel::reset();
    std::copy(_initial.begin(), _initial.end(), _state.begin());
}

void OnOffInjectionModel::display(std::ostream &os) const {
    RandomInjectionModel::display(os);
    os << "-----------------------------------------------\n";
    os <<   "alpha              : " << _alpha
       << "\nbeta               : " << _beta
       << "\naverage burst size : " << _burst_size_avg
       << "\ninitial state      : " << _initial
       << "\nstate              : " << _state
       << "\n";
}
std::ostream &OnOffInjectionModel::operator<<(std::ostream &os) const {
  RandomInjectionModel::operator<<(os);
  os << "\n-----------------------------------------------\n";
  os <<   "alpha              : " << _alpha
     << "\nbeta               : " << _beta
     << "\naverage burst size : " << _burst_size_avg
     << "\ninitial state      : " << _initial
     << "\nstate              : " << _state;
  return os;
}

InjectionState OnOffInjectionModel::inject(int source) {
    if (source < 0 || source >= _num_nodes) {
      std::cerr << "Argument source should be an integer within the range of [0, " << _num_nodes << ").\n";
      return InjectionState::OFF;
    }

    // state transition
    _state[source] =
        _state[source] ? (_distribution(_eng) >= _alpha) : (_distribution(_eng) < _beta);

    return (_state[source] ? InjectionState::ON : InjectionState::OFF);
}

// Implementation for ExtendedOnOffInjectionModel
// ////////////////////////////////////////////////////////////////////////////////////
ExtendedOnOffInjection::ExtendedOnOffInjection(int num_nodes,
                                               double rate,
                                               std::mt19937::result_type seed,
                                               double alpha,
                                               double beta,
                                               double burst_size_avg,
                                               const std::vector<bool> &initial) :
    RandomInjectionModel(num_nodes, rate, seed, "ExOnOff"), _alpha(alpha), _beta(beta),
    _burst_size_avg(burst_size_avg),
    _initial(initial), _state(initial) {
  assert((alpha == -1 && beta == -1 && burst_size_avg > 1) ||
      (alpha == -1 && beta >= 0 && beta < 1.0 && burst_size_avg == -1) ||
      (alpha > 0 && alpha <= 1.0 && beta == -1 && burst_size_avg == -1) &&
          "Exact one argument of alpha, beta, or burst_size_avg should be set. ");
  double on_off_ratio = _rate / (1.0 - _rate);
  if (alpha != -1) {
    _burst_size_avg = 1.0 / _alpha;
    auto x = (1.0 - _alpha) / _alpha;
    _beta = on_off_ratio / (on_off_ratio + x);
  } else if (beta != -1) {
    double off_size = (1.0 - _beta) / _beta;
    _alpha = 1.0 / (1 + on_off_ratio * off_size);
    _burst_size_avg = on_off_ratio * off_size + 1;
  } else {
    _alpha = 1.0 / _burst_size_avg;
    auto x = (1.0 - _alpha) / _alpha;
    _beta = on_off_ratio / (on_off_ratio + x);
  }

  assert(_alpha < 1.0 && _beta < 1.0 && "alpha and beta can not be larger than 1.0!");
}

void ExtendedOnOffInjection::reset() {
      RandomInjectionModel::reset();
      std::copy(_initial.begin(), _initial.end(), _state.begin());
}

void ExtendedOnOffInjection::display(std::ostream &os) const {
  RandomInjectionModel::display(os);
  os << "-----------------------------------------------\n";
  os <<   "alpha              : " << _alpha
     << "\nbeta               : " << _beta
     << "\naverage burst size : " << _burst_size_avg
     << "\ninitial state      : " << _initial
     << "\nstate              : " << _state
     << "\n";
}

std::ostream& ExtendedOnOffInjection::operator<<(std::ostream &os) const {
  RandomInjectionModel::operator<<(os);
  os << "\n-----------------------------------------------\n";
  os <<   "alpha              : " << _alpha
     << "\nbeta               : " << _beta
     << "\naverage burst size : " << _burst_size_avg
     << "\ninitial state      : " << _initial
     << "\nstate              : " << _state;
  return os;
}

InjectionState ExtendedOnOffInjection::inject(int source) {
    if (source < 0 || source >= _num_nodes) {
      std::cerr << "Argument source should be an integer within the range of [0, " << _num_nodes << ").\n";
      return InjectionState::OFF;
    }

    // state transition
    bool state_bk;
    bool changed = false;
    do {
      state_bk = _state[source];
      _state[source] =
          _state[source] ? (_distribution(_eng) >= _alpha) : (_distribution(_eng) < _beta);
      if (!state_bk && _state[source])
        changed = true;
    } while (_state[source] != state_bk);

    return (_state[source] ? (changed ? InjectionState::ON_AND_CHANGE : InjectionState::ON) : InjectionState::OFF);
}

} // namespace saber
