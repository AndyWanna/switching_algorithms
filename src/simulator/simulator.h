
#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <map>
#include <nlohmann/json.hpp>
#include <cmath>


#include <traffic_generator/traffic_pattern_factory.h>
#include <traffic_generator/injection_model_factory.h>
#include <switch/iq_switch_factory.h>
#include <stats.hpp>
#include <utils.hpp>

using nlohmann::json;
namespace saber {
class SimulatorFactory;
// Class for simulator utilities
// ///////////////////////////////////////////////////////////////////////////////////////////////
class SimUtil {
 public:
  static int next;
  static std::vector<unsigned> seeds;
  static unsigned get_seed() {
    assert(!seeds.empty());
    return seeds[(next++) % seeds.size()];
  }
};
// Base class for Simulator
// /////////////////////////////////////////////////////////////////////////////////////////////////////
class Simulator {
  friend class SimulatorFactory;
 protected:
  Simulator(std::string name, int verbose) : _id(ID++), _name(std::move(name)), _verbose(verbose) {}
 public:
  // virtual functions
  virtual ~Simulator() = default;;
  virtual void simulate() = 0;
  virtual const json& get_stats() const = 0;
  virtual void reset() {}
  virtual void display_stats(std::ostream &os) const = 0;
  virtual void display(std::ostream& os) const {
    os <<   "name           : " << _name
       << "\nid             : " << _id
       << "\nverbose level  : " << _verbose
       << "\n";
  }

  // basic functions
  int id() { return _id; }
  std::string name() { return _name; }
  int verbose() { return _verbose; }
 protected:
  static int ID;
  const int _id;
  const std::string _name;
  const int _verbose{0};
};

//! Class for Input-Queued Switch Simulator
class IQSwitchSimulator : public Simulator {
  friend class SimulatorFactory;
 protected:
  const std::string _stopping_rule{"relative"};
  IQSwitchSimulator(std::string name, int verbose, int num_inputs, int num_outputs, const json &conf);
  // function to configure traffic
  void load_traffic_profile(const json &conf);
  void create_bernoulli_injection(unsigned seed) ;
  void create_onoff_injection(unsigned seed) ;
  void configure_switch(const json &conf) ;
  void init_stats(const json &in_conf) ;
  void init_channels() ;
  void clear_channels() ;
  /** @brief Stopping rule
   *
   * This function implements the stopping rul for MCMC simulations in
   * - The Asymptotic Validity of Sequential Stopping Rules for Stochastic Simulations, by P. W. Glynn and W. Whitt
   * - Batch means and spectral variance estimators in Markov chain Monte Carlo, by James M. Flegal, Galin L. Jones
   *
   * @param n               number of steps simulated
   * @param stddev_est
   * @return
   */
  bool stop_now(size_t n, double stddev_est, double average = -1) const ;
  bool fix_stopping_rule(size_t n, double stddev_est) const ;
  bool rel_stopping_rule(size_t n, double stddev_est, double average) const ;

  int arriving(InjectionModel *injection, TrafficPattern *traffic) ;
  int departing(const IQSwitch *sw) ;
  bool simulate_on(InjectionModel *injection, TrafficPattern *traffic, bool first_call = false) ;

 public:
  ~IQSwitchSimulator() override ;

  void simulate() override ;
  void reset() override ;
  void display_stats(std::ostream &os) const override ;
  const json& get_stats() const override {
    return _stats_results;
  }

  void display(std::ostream &os) const override {
    Simulator::display(os);
    os << "-----------------------------------------------------------------";
    os << "\nleast simulation effort      : " << _least_simulation_effort
       << "\nmost simulation effort       : " << _most_simulation_effort
       << "\nerror bound (for stopping)   : " << _error_bound
       << "\ntraffic patterns             : " << _traffic_pattern_names
       << "\n";
    if(_switch != nullptr)_switch->display(os);
  }

 protected:
  //
  const int _num_inputs{-1};
  const int _num_outputs{-1};
  // Default values for simulation efforts
  const int MIN_EFFORT_C{100};
  const int MAX_EFFORT_C{5000};
  // added @02/03/2019
  double _max_mem_ratio{0.618};
  const unsigned long long TOT_MEM;
  // stopping related parameter
  int _least_simulation_effort{-1};
  int _most_simulation_effort{-1};
  double _error_bound{0.01};
  double _student_t{2.326};// 98% two-sided student t for n = infinity
  // parameters
  std::vector<double> _loads{};
  std::vector<std::string> _traffic_pattern_names{};
  std::vector<double> _burst_sizes{};
  // status
  bool _channel_installed{false};
  bool _reset{false};
  double _current_load{-1};
  double _current_burst_size{-1};
  // components
  IQSwitch *_switch{nullptr};
  std::string _switch_type{"generic"};
  std::vector<Channel<Packet> *> _input_channels;
  std::vector<Channel<Packet> *> _output_channels;
  std::map<std::pair<double, double>, InjectionModel *> _injections;
  std::map<std::string, TrafficPattern *> _traffic_patterns;
  // changed to _instruments @2019-01-02
  //  Stats *_delay_stats;
  //  Stats *_queue_length_stats;
  //
  std::map<std::string, Stats *> _instruments;
  // final results
  json _stats_results;
};
} // namespace saber

#endif // SIMULATOR_H

