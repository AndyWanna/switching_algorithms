
#ifndef ONOFF_INJECTION_MODEL_H
#define ONOFF_INJECTION_MODEL_H

#include "injection_model.h"


namespace saber {
// Class for on off injection
// Please refer to <a HREF="http://www-bcf.usc.edu/~mjneely/pdf_papers/maximal-delay-ton.pdf">Delay Analysis for
// Maximal Scheduling with Flow Control in Wireless Networks with Bursty Traffic</a>
// for more details.
// ///////////////////////////////////////////////////////////////////////////////////////////////////////
class OnOffInjectionModel : public RandomInjectionModel {
  friend class InjectionModelFactory;
 protected:
  std::uniform_real_distribution<double> _distribution{0, 1.0};
  double _alpha{0.0};// transition probability from ON to OFF
  double _beta{0.0};//  transition probability from OFF to ON
  double _burst_size_avg{0.0};// average burst size
  std::vector<bool> _initial;
  std::vector<bool> _state;

  /** @brief Constructor for class OnOffInjectionModel
   *
   *
   * @param num_nodes               @see #InjectionModel
   * @param rate                    @see #InjectionModel
   * @param seed                    @see RandomInjectionModel
   *
   * @param alpha                   Double Transition probability from state ON to OFF
   * @param beta                    Double Transition probability from state OFF to ON
   *
   * @param burst_size_avg          Double Average burst size
   * @param initial                 std::vector<bool> Initial states (false = OFF, true = ON)
   */
  OnOffInjectionModel(int num_nodes, double rate, std::mt19937::result_type seed, double alpha, double beta,
                      double burst_size_avg, const std::vector<bool> &initial) ;
 public:
  ~OnOffInjectionModel() override = default;
  void reset() override ;

  void display(std::ostream &os) const override ;
  std::ostream&operator<<(std::ostream& os) const override;

  InjectionState inject(int source) override ;
}; // OnOffInjectionModel


// Class for Extended On Off Injection
// Please refer to <a HREF="https://dl.acm.org/citation.cfm?id=3084440">
// Queue-Proportional Sampling: A Better Approach to Crossbar Scheduling for Input-Queued Switches</a> for more details.
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ExtendedOnOffInjection : public RandomInjectionModel {
  friend class InjectionModelFactory;
 protected:
  std::uniform_real_distribution<double> _distribution{0, 1.0};
  double _alpha{0.0};// transition probability from ON to OFF
  double _beta{0.0};//  transition probability from OFF to ON
  double _burst_size_avg{0.0};// average burst size
  std::vector<bool> _initial;
  std::vector<bool> _state;

  ExtendedOnOffInjection(int num_nodes, double rate, std::mt19937::result_type seed, double alpha, double beta,
                         double burst_size_avg, const std::vector<bool> &initial);

 public:
  ~ExtendedOnOffInjection() override = default;
  void reset() override ;

  void display(std::ostream &os) const override ;

  std::ostream& operator<<(std::ostream& os) const override;
  InjectionState inject(int source) override ;
};
} // namespace saber
#endif // ONOFF_INJECTION_MODEL_H
