//
#include "injection_model.h"
#include "utils.hpp"

namespace saber {

// Implementation of class InjectionModel
//  ////////////////////////////////////////////////////////////////////////////////////////////////////

int InjectionModel::ID = 1;// Initialize class variable

InjectionModel::InjectionModel(int num_nodes, double rate, std::string name) :
    _id(ID++), _name(std::move(name)), _num_nodes(num_nodes), _rate(rate) {
  assert(num_nodes > 0 && "Number of nodes should be a positive integer.");
  assert(rate > 0 && rate <= 1.0 && "Injection rate should a positive number with in the range from 0 to 1.0.");
}

std::string InjectionModel::name() const {
  return _name;
}

int InjectionModel::id() const {
  return _id;
}

int InjectionModel::num_nodes() const {
  return _num_nodes;
}

double InjectionModel::rate() const {
  return _rate;
}
void InjectionModel::display(std::ostream &os) const {
  os <<   "name      : " << name()
     << "\nid        : " << id()
     << "\nnum_nodes : " << num_nodes()
     << "\nrate      : " << rate()
     << "\n";
}
std::ostream &InjectionModel::operator<<(std::ostream &os) const {
  os <<   "name      : " << name()
     << "\nid        : " << id()
     << "\nnum_nodes : " << num_nodes()
     << "\nrate      : " << rate();
  return os;
}


// Implementation for class RandomInjectionModel
// ////////////////////////////////////////////////////////////////////////////////////
RandomInjectionModel::RandomInjectionModel(int num_nodes,
                                           double rate,
                                           std::mt19937::result_type seed,
                                           const std::string &name) : InjectionModel(num_nodes, rate, name),
                                                                      _seed(seed),
                                                                      _eng(seed) {}
void RandomInjectionModel::display(std::ostream &os) const {
  InjectionModel::display(os);
  os << "seed      : " << seed() << "\n";
}
std::mt19937::result_type RandomInjectionModel::seed() const {
  return _seed;
}
std::ostream &RandomInjectionModel::operator<<(std::ostream &os) const {
  InjectionModel::operator<<(os);
  os << "\nseed      : " << seed();
  return os;
}

} // namespace saber
