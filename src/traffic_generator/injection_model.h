// injection_model.h header file

#ifndef _INJECTION_MODEL_H_
#define _INJECTION_MODEL_H_

#include <random>
#include <cassert>
#include "utils.hpp"

namespace saber {

// States for injection models
enum InjectionState { OFF = 0, ON = 1, ON_AND_CHANGE = 2 };


// Declaration of class InjectionModelFactory
class InjectionModelFactory;

// Base class InjectionModel for a homogeneous injections for a system that has multiple interfaces
// ////////////////////////////////////////////////////////////////////////////////////////////////
class InjectionModel {
  friend class InjectionModelFactory;
 protected:
  static int ID; // class variable

  const int _id; // unique immutable id for an injection model object (reserved for future use)
  const std::string _name; // unique immutable name for an injection model object


  int _num_nodes; // Number of input interfaces of the system, for which the injection model is built
  double _rate; // Rate for each interface (same for all interfaces)

  /** @brief Constructor of class InjectionModel
   *
   * This constructor builds a homogeneous injection model for a system
   * that has multiple input interfaces, such a switching systems. Here
   * by "homogeneous", we mean all the interfaces will have the same injection
   * rate.
   *
   * The variable #_num_nodes is somehow redundant, we might remove it in the future.
   * Note that the member inject() of class InjectionModel uses variable #_num_nodes
   * to validate the argument of inject(), i.e., the identity of the interface for
   * which the traffic is injected.
   *
   * @param num_nodes   Integer     Number of input interfaces (MUST be positive)
   * @param rate        Double      Injection rate (a real number between 0 and 1)
   * @param name        String      Name of the injection model
   */
  InjectionModel(int num_nodes, double rate, std::string name = "InjectionBase") ;

 public:
  virtual ~InjectionModel() = default;

  // "Get" functions
  std::string name() const ;
  int id() const ;
  int num_nodes() const ;
  double rate() const ;

  // Virtual functions
  virtual void reset() {}

  /** @brief  Try to inject traffic for an interface
   *
   * Here #source seems to be unnecessary for homogeneous systems, however
   * we keep for 1) making this virtual function also work for heterogeneous
   * systems; 2) making this API more meaningful, i.e., it tells the user
   * the interface for which the traffic is injected.
   *
   * @param source  Integer Identity of the interface, for which the traffic is injected
   * @return enum InjectionState
   */
  virtual InjectionState inject(int source) = 0;
  virtual void display(std::ostream &os) const ;
  // added @12-29-18
  virtual std::ostream& operator<<(std::ostream& os) const ;
};


// Base class for randomized injection model
// ////////////////////////////////////////////////////////////////////////////////////////////////
class RandomInjectionModel : public InjectionModel {
  friend class InjectionModelFactory;
 protected:

  const std::mt19937::result_type _seed{0};
  std::mt19937 _eng{std::random_device{}()};

  /** @brief Constructor for class RandomInjectionModel
   *
   *
   * @see #InjectionModel
   */
  RandomInjectionModel(int num_nodes,
                       double rate,
                       std::mt19937::result_type seed,
                       const std::string &name = "RandomInjectionBase") ;

 public:
  ~RandomInjectionModel() override = default;
  void reset() override {}

  void display(std::ostream &os) const override ;
  InjectionState inject(int source) override = 0;
  std::mt19937::result_type seed() const;
  std::ostream& operator<<(std::ostream& os) const override;
};

}
#endif //_INJECTION_MODEL_HPP_

