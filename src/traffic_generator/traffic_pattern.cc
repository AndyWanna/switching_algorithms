//
#include "traffic_pattern.h"

namespace saber{
int TrafficPattern::ID = 0;
// Implementation for class TrafficPattern
// //////////////////////////////////////////////////////////////////////
TrafficPattern::TrafficPattern(int num_nodes, std::string name) :
    _id(ID++), _name(std::move(name)), _num_nodes(num_nodes) {
  assert(num_nodes > 0 && "num_nodes should be a positive integer");
}
std::string TrafficPattern::name() const {
  return _name;
}
int TrafficPattern::id() const {
  return _id;
}
int TrafficPattern::num_nodes() const {
  return _num_nodes;
}
void TrafficPattern::display(std::ostream &os) const {
    os <<   "name       : " << name()
       << "\nid         : " << id()
       << "\nnum_nodes  : " << num_nodes()
       << "\n";
}
std::ostream &TrafficPattern::operator<<(std::ostream &os) const {
  os <<   "name       : " << name()
     << "\nid         : " << id()
     << "\nnum_nodes  : " << num_nodes();
  return os;
}

// Implementation for class RandomTrafficPattern
// /////////////////////////////////////////////////////////////////////////////////////////////////////////
RandomTrafficPattern::RandomTrafficPattern(int num_nodes, std::mt19937::result_type seed, std::string name) :
    TrafficPattern(num_nodes, std::move(name)), _seed(seed), _eng(seed) {}
std::ostream &RandomTrafficPattern::operator<<(std::ostream &os) const {
  TrafficPattern::operator<<(os);
  os << "\nseed       : " << _seed;
  return os;
}
void RandomTrafficPattern::display(std::ostream &os) const {
  TrafficPattern::display(os);
  os << "seed       : " << _seed
     << "\n";
}



} // namespace saber