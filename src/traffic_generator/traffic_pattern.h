//  traff_pattern.h header file
#ifndef TRAFFIC_PATTERN_H
#define TRAFFIC_PATTERN_H

#include <cassert>
#include <random>
#include "utils.hpp"

namespace saber {
class TrafficPatternFactory;

// Base class for traffic pattern
// /////////////////////////////////////////////////////////////////////////////////////////
class TrafficPattern {
  /*!
   * Declare TrafficPatternFactory as a friend class of TrafficPattern
   * so that the former can access the "hidden constructor" of the latter.
   */
  friend class TrafficPatternFactory;
 protected:
  static int ID;
  const int _id;
  const std::string _name;
  int _num_nodes;
  //! Hidden constructor
  explicit TrafficPattern(int num_nodes, std::string name = "TrafficPatternBase") ;

 public:
  std::string name() const ;
  int id() const ;
  int num_nodes() const ;
  virtual ~TrafficPattern() = default;
  virtual void reset() {};
  virtual int destination(int source) = 0;
  virtual void display(std::ostream &os) const ;
  virtual std::ostream& operator<<(std::ostream& os) const;
};

// Random Traffic Pattern
///////////////////////////////////////////////////////////////////////////////////////
class RandomTrafficPattern : public TrafficPattern {
  friend class TrafficPatternFactory;
 protected:
  const std::mt19937::result_type _seed;
  std::mt19937 _eng{std::random_device{}()};
  RandomTrafficPattern(int num_nodes,
                       std::mt19937::result_type seed,
                       std::string name = "RandomTrafficPatternBase") ;

 public:
  ~RandomTrafficPattern() override = default;
  void reset() override {}
  int destination(int source) override = 0;
  void display(std::ostream &os) const override ;
  std::ostream& operator<<(std::ostream& os) const override ;
};

} // namespace saber

#endif // TRAFFIC_PATTERN_H

