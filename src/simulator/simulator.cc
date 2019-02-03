
#include <climits>

#include <exceptions.hpp>
#include "simulator.h"
#include <mem_utils.hpp>
namespace saber {
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////              Static Variable Initialization      ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
int SimUtil::next = 0;
std::vector<unsigned> SimUtil::seeds =
    std::vector<unsigned>({1186843690L, 2943691978L, 3127594590L, 2988723138L, 2650238383L, 3575644507L, 2231556358L,
                      768359651L, 2312418790L, 1995961490L, 358037056L, 3585827518L, 1007959227L, 76333057L,
                      3019632670L, 443055176L, 1015913762L, 501895793L, 1543682012L, 93915144L});


// Implementation of class IQSwitchSimulator
// /////////////////////////////////////////////////////////////////////////////////////////////////////////
int Simulator::ID = 0;
IQSwitchSimulator::IQSwitchSimulator(std::string name,
                                     int verbose,
                                     int num_inputs,
                                     int num_outputs,
                                     const json &conf) : Simulator(std::move(name), verbose), _num_inputs(num_inputs),
                                                         _num_outputs(num_outputs),
                                                         TOT_MEM(getTotalSystemMemory()),
                                                         _input_channels(num_inputs, nullptr),
                                                         _output_channels(num_outputs, nullptr) {
  long long n_sq = num_inputs * num_outputs;
  if (conf.count("least_simulation_efforts"))
    _least_simulation_effort = conf["least_simulation_efforts"].get<int>();
  else {
    if (INT_MAX / (n_sq) < MIN_EFFORT_C) {
      std::cerr << "WARNING: Default least simulation effort is out of range, thus setting to INT_MAX!\n";
      _least_simulation_effort = INT_MAX;
    } else {
      _least_simulation_effort = static_cast<int>(n_sq * MIN_EFFORT_C);
    }
  }

  if (conf.count("most_simulation_efforts"))
    _most_simulation_effort = conf["most_simulation_efforts"].get<int>();
  else {
    if (INT_MAX / (n_sq) < MAX_EFFORT_C) {
      std::cerr << "WARNING: Default most simulation effort is out of range, thus setting to INT_MAX!\n";
      _least_simulation_effort = INT_MAX;
    } else {
      _least_simulation_effort = static_cast<int>(n_sq * MAX_EFFORT_C);
    }
  }
  // initialize channels
  init_channels();
  // create a switch
  configure_switch(conf);
  _switch->install_channels(_input_channels, _output_channels);
  // create traffic related objects
  load_traffic_profile(conf);
  // initialize statistic objects
  init_stats(conf);
  // mark as initialized
  _reset = true;
}
void IQSwitchSimulator::load_traffic_profile(const json &conf) {
  if(!(conf.count("load"))) throw MissingArgumentException("Argument conf MUST contain \"load\"!");
  if(!(conf.count("traffic_patterns"))) throw MissingArgumentException("Argument conf MUST contain \"traffic_patterns\"!");
  if(!(conf.count("injection"))) throw MissingArgumentException("Argument conf MUST contain \"injection\"!");

  _loads = conf["load"].get<std::vector<double>>();
  _traffic_pattern_names = conf["traffic_patterns"].get<std::vector<std::string> >();
  auto injection_name = conf["injection"].get<std::string>();

  json t_conf = {
      {"name", "uniform"},
      {"num_nodes", _num_outputs},
      {"seed", SimUtil::get_seed()}
  };

  if (conf.count("seeds") && conf["seeds"].count("traffic_pattern")) {
    t_conf["seed"] = conf["seeds"]["traffic_pattern"].get<unsigned>();
  }

  for (const auto &tm_name : _traffic_pattern_names) {
    t_conf["name"] = tm_name;
    _traffic_patterns[tm_name] = TrafficPatternFactory::Create(t_conf);
  }

  unsigned injection_seed = SimUtil::get_seed();
  if (conf.count("seeds") && conf["seeds"].count(injection_name)) {
    injection_seed = conf["seeds"][injection_name].get<unsigned>();
  }
  if (injection_name.find("onoff") != std::string::npos) {
    if(!(conf.count("burst_size"))) throw MissingArgumentException("Argument conf MUST contain \"burst_size\" for on off injections!");
    _burst_sizes = conf["burst_size"].get<std::vector<double>>();
    create_onoff_injection(injection_seed);
  } else {
    create_bernoulli_injection(injection_seed);
  }
}
void IQSwitchSimulator::create_bernoulli_injection(unsigned seed) {
  json conf = {
      {"name", "bernoulli"},
      {"num_nodes", _num_outputs},
      {"seed", seed},
      {"rate", -1}
  };
  for (const auto &load : _loads) {
    conf["rate"] = load;
    _injections[{load, -1}] = InjectionModelFactory::Create(conf);
  }
}
void IQSwitchSimulator::create_onoff_injection(unsigned seed) {
  json conf = {
      {"name", "onoff"},
      {"num_nodes", _num_outputs},
      {"rate", -1},
      {"seed", seed},
      {"burst_size_avg", -1.0}
  };
  if(_burst_sizes.empty() || _loads.empty()) throw EmptyArgumentException("Both _burst_sizes and _loads CANNOT be empty for create on off injections!");
  for (const auto &bs : _burst_sizes) {
    conf["burst_size_avg"] = bs;
    for (const auto &load : _loads) {
      conf["rate"] = load;
      _injections[{load, bs}] = InjectionModelFactory::Create(conf);
    }
  }
}
void IQSwitchSimulator::configure_switch(const json &conf) {
  json switch_conf = {
      {"num_inputs", _num_inputs},
      {"num_outputs", _num_outputs}
  };
  json scheduler_conf = {
      {"num_inputs", _num_inputs},
      {"num_outputs", _num_outputs}
  };
  // FIXED: forget to initialize the value of switch type
  if(conf["switch"].count("type")) _switch_type = conf["switch"]["type"];
  for (json::const_iterator it = conf["switch"].cbegin(), e_it = conf["switch"].cend(); it != e_it; ++it) {
    switch_conf[it.key()] = it.value();
  }
  for (json::const_iterator it = conf["scheduler"].cbegin(), e_it = conf["scheduler"].cend(); it != e_it; ++it) {
    scheduler_conf[it.key()] = it.value();
  }
#ifdef DEBUG
    std::cerr << "Switch configuration : \n" << switch_conf << std::endl;
    std::cerr << "Scheduler configuration : \n" << scheduler_conf << std::endl;
#endif
  _switch = IQSwitchFactory::Create(switch_conf, scheduler_conf);
}

// currently only support for throughput, delay and queue length
// but you can add support for other instruments here
void IQSwitchSimulator::init_stats(const json &in_conf) {
  std::set<std::string> instruments ;
  if (!in_conf.count("instruments")) {
    if(_verbose > 0) std::cout << "No \"instruments\" are provided, use the default one, i.e., the delay & throughput instruments!\n";
    if(in_conf.count("switch") && in_conf["switch"].count("type") && in_conf["switch"]["type"] != "simplified") instruments.insert("delay");
    instruments.insert("totalQueueLength");
  }
  else {
    for (const auto & ins : in_conf["instruments"].get<std::vector<std::string>>()) instruments.insert(ins);
  }
  bool throughput = false;
  json ins_conf = {
      {"name", "general"}
  };
  for (const auto& ins : instruments) {
    if (ins == "throughput") continue;
    if(ins == "delay" || ins == "totalQueueLength") {
      ins_conf["name"] = ins;
      _instruments[ins] = Stats::New(ins_conf);
      if(!throughput) {
        _instruments[ins]->add_simple_counter("total_arrivals");
        _instruments[ins]->add_simple_counter("total_departures");
        _instruments["throughput"] = _instruments[ins];// share
        throughput = true;
      }
    }  else {
      throw UnknownParameterException("Instrument " + ins + " has not been supported yet!");
    }
  }
}

void IQSwitchSimulator::init_channels() {
  if(_channel_installed) throw ReInitializationException("It seems channels have already been installed. You CANNOT install them again, as channels CAN only be installed once!");
  json conf = {
      {"name", ""}
  };
  for (int i = 0; i < _num_inputs; ++i) {
    conf["name"] = "input_channel_" + std::to_string(i);
    _input_channels[i] = Channel<Packet>::New(conf);
  }
  for (int i = 0; i < _num_outputs; ++i) {
    conf["name"] = "output_channel_" + std::to_string(i);
    _output_channels[i] = Channel<Packet>::New(conf);
  }
  _channel_installed = true;
}
void IQSwitchSimulator::clear_channels() {
  if(!_channel_installed) throw DeleteNonInitializedException("You are trying to delete a vector of non-initialized pointers!");

  for (int i = 0; i < _num_inputs; ++i) {
    while (!_input_channels[i]->empty()) {
      Packet *pkt = _input_channels[i]->receive();
      delete (pkt);
    }
    delete (_input_channels[i]);
  }
  for (int i = 0; i < _num_outputs; ++i) {
    while (!_output_channels[i]->empty()) {
      Packet *pkt = _output_channels[i]->receive();
      delete (pkt);
    }
    delete (_output_channels[i]);
  }
  _channel_installed = false;
}

bool IQSwitchSimulator::stop_now(size_t n, double stddev_est, double average) const {
  if(_stopping_rule == "fixed") return fix_stopping_rule(n, stddev_est);
  else return rel_stopping_rule(n, stddev_est, average);
}
bool IQSwitchSimulator::fix_stopping_rule(size_t n, double stddev_est) const {
  if (n <= _least_simulation_effort)
    return false;
  if (n == _most_simulation_effort) {
    return true;
  }
  return (_student_t * stddev_est / std::sqrt(n) + 1.0 / n < _error_bound);
}
bool IQSwitchSimulator::rel_stopping_rule(size_t n, double stddev_est, double average) const {
  if (n <= _least_simulation_effort)
    return false;
  if (n == _most_simulation_effort) {
    return true;
  }
  return (_student_t * stddev_est / std::sqrt(n) + 1.0 / n < (_error_bound * average));
}
int IQSwitchSimulator::arriving(InjectionModel *injection, TrafficPattern *traffic) {
  int num_arrivals = 0;
  for (int s = 0; s < _num_inputs; ++s) {
    if (injection->inject(s)) {
      int d = traffic->destination(s);
      assert(d >= 0 && d < _num_outputs);
      _input_channels[s]->send(new Packet(s, d));
      ++num_arrivals;
    }
  }
  return num_arrivals;
}
int IQSwitchSimulator::departing(const IQSwitch *sw) {
  int num_departures = 0;
  int delay;

  if (_switch_type == "generic") {
    for (int d = 0; d < _num_outputs; ++d) {
      assert (_output_channels[d]->size() <= 1);
      if (!_output_channels[d]->empty()) {
        Packet *pkt = _output_channels[d]->receive();
        delay = pkt->get_departure_time() - pkt->get_arrival_time();
        // _delay_stats->add_sample(delay);
        if(_instruments.count("delay")) _instruments["delay"]->add_sample(delay);
        ++num_departures;
        delete (pkt);// release memory of pkt
      }
    }
  } else if(_switch_type == "simplified") {
    const auto& departs = sw->get_departures();
    for(const auto& sdp : departs) {
      if(sdp.first != -1) ++ num_departures;
      else break;
    }
  }

  return num_departures;
}

bool IQSwitchSimulator::simulate_on(InjectionModel *injection, TrafficPattern *traffic, bool first_call) {
  // assume that all component have been reset
  assert(_reset);
  _reset = false;// mark that the status of the switch is dirty

  const int width = 18;
  const int precision = 6;
  const std::string switch_size = std::to_string(_num_inputs) + "X" + std::to_string(_num_outputs);
  if (first_call && _verbose >= 0) {
    std::cout << _switch->get_scheduler()->name() + "|" + injection->name() + "\n";

    if (!_burst_sizes.empty()) {
      StdOutUtil::print_fw<width, precision>("#Load",
                                             "SwitchSize",
                                             "traffic-model",
                                             "burst-size",
                                             "throughput",
                                             "mean-delay",
                                             "delay-var",
                                             "tot-qLen",
                                             "tot-qLen-var");
    } else {
      StdOutUtil::print_fw<width, precision>("#Load",
                                             "SwitchSize",
                                             "traffic-model",
                                             "throughput",
                                             "mean-delay",
                                             "delay-var",
                                             "tot-qLen",
                                             "tot-qLen-var");
    }
    std::cout << std::endl;
  }

  // initialize _stats_results
  if (first_call) {
    _stats_results["experiment_name"] = _switch->get_scheduler()->name() + "|" + injection->name();
    _stats_results["switch_size"] = switch_size;
    _stats_results[traffic->name()]["load"] = std::vector<double>();
    _stats_results[traffic->name()]["throughput"] = std::vector<double>();
    _stats_results[traffic->name()]["mean-delay"] = std::vector<double>();
    _stats_results[traffic->name()]["delay-variance"] = std::vector<double>();
    //_stats_results[traffic->name()]["mean-queue-length"] = std::vector<double>();
    if (!_burst_sizes.empty())
      _stats_results[traffic->name()]["burst-size"] = std::vector<double>();
    _stats_results[traffic->name()]["total-queue-length-average"] = std::vector<double>();
    _stats_results[traffic->name()]["total-queue-length-variance"] = std::vector<double>();
    _stats_results[traffic->name()]["entered_steady_stage"] = std::vector<bool>();
  }

  size_t n = 0;

  double NaN = std::numeric_limits<double>::quiet_NaN();
  int num_stall_packets = 0;
  int num_arrivals, num_departures;
  size_t delay = 0;
  const int print_interval = (_least_simulation_effort / 4);
  double stddev_est = NaN;
  double avg_est = NaN;

  // The following several lines are added for debug purpose
  // auto voqs = _switch->get_queue_matrix();
  for (int s = 0; s < _num_inputs; ++s) {
    for (int d = 0; d < _num_outputs; ++d) {
      if(_switch->get_queue_length(s, d) != 0)  throw NonEmptyInitializationException("All VOQs should be empty at the beginning.\n");
    }
  }
  for (int s = 0; s < _num_inputs; ++s) {
    if(!(_input_channels[s]->empty())) throw NonEmptyInitializationException("All input channels should be empty at the beginning.\n");
  }
  for (int d = 0; d < _num_outputs; ++d) {
    if(!(_output_channels[d]->empty())) throw NonEmptyInitializationException("All output channels should be empty at the beginning.\n");
  }

  bool has_total_queue_len_ins = (_instruments.count("totalQueueLength") > 0);
  bool has_delay_ins = (_instruments.count("delay") > 0);
  std::string stopping_ins = (has_delay_ins?"delay":"totalQueueLength");
  auto &throughput_ins = _instruments["throughput"];

  const unsigned long long pkt_size = sizeof(Packet) * 8; // size of each packet
  const double max_num_pkts = (TOT_MEM * _max_mem_ratio) / pkt_size;

  while (!stop_now(n, stddev_est, avg_est)) {
    // arrival
    num_arrivals = arriving(injection, traffic);
    num_stall_packets += num_arrivals;

    throughput_ins->increment_simple_counter("total_arrivals", num_arrivals);
    // switching
    _switch->switching();
    // departure
    num_departures = departing(_switch);
    num_stall_packets -= num_departures;
    // update statistics
    throughput_ins->increment_simple_counter("total_departures", num_departures);
    if(has_total_queue_len_ins) _instruments["totalQueueLength"]->add_sample(num_stall_packets);
    if (n >= _least_simulation_effort) {
      stddev_est = std::sqrt(_instruments[stopping_ins]->variance());
      avg_est = _instruments[stopping_ins]->average();
    }
    ++n;
    if (n % print_interval == 0 && _verbose >= 0) {
      if (!_burst_sizes.empty()) {
        StdOutUtil::print_fw<width, precision>(_current_load,
                                               switch_size,
                                               traffic->name(),
                                               _current_burst_size,
                                               (throughput_ins->get_counter("total_departures")
                                                   / throughput_ins->get_counter("total_arrivals")),
                                               (has_delay_ins?_instruments["delay"]->average():NaN),
                                               (has_delay_ins?_instruments["delay"]->variance():NaN),
                                               (has_total_queue_len_ins?_instruments["totalQueueLength"]->average():NaN),
                                               (has_total_queue_len_ins?_instruments["totalQueueLength"]->variance():NaN));
      } else {
        StdOutUtil::print_fw<width, precision>(_current_load,
                                               switch_size,
                                               traffic->name(),
                                               (throughput_ins->get_counter("total_departures")
                                                   / throughput_ins->get_counter("total_arrivals")),
                                               (has_delay_ins?_instruments["delay"]->average():NaN),
                                               (has_delay_ins?_instruments["delay"]->variance():NaN),
                                               (has_total_queue_len_ins?_instruments["totalQueueLength"]->average():NaN),
                                               (has_total_queue_len_ins?_instruments["totalQueueLength"]->variance():NaN));
      }
      std::cout << "\n";
    }
    if (n > _least_simulation_effort && num_stall_packets > max_num_pkts) {
      // protect the system from draining out of memory
      break;
    }
  }


  bool entered_steady_stage = (n < _most_simulation_effort);
  // record final results
  _stats_results[traffic->name()]["load"].push_back(_current_load);
  _stats_results[traffic->name()]["throughput"]
      .push_back((throughput_ins->get_counter("total_departures") / throughput_ins->get_counter("total_arrivals")));
  _stats_results[traffic->name()]["mean-delay"].push_back(has_delay_ins?_instruments["delay"]->average():NaN);
  _stats_results[traffic->name()]["delay-variance"].push_back(has_delay_ins?_instruments["delay"]->variance():NaN);
  _stats_results[traffic->name()]["total-queue-length-average"].push_back((has_total_queue_len_ins?_instruments["totalQueueLength"]->average():NaN));
  _stats_results[traffic->name()]["total-queue-length-variance"].push_back((has_total_queue_len_ins?_instruments["totalQueueLength"]->variance():NaN));
  if (!_burst_sizes.empty())
    _stats_results[traffic->name()]["burst-size"].push_back(_current_burst_size);
  // added @01/20/2019
  _stats_results[traffic->name()]["entered_steady_stage"].push_back(entered_steady_stage);

  return entered_steady_stage; // consider as unstable (not able to enter the steady stage) when n is too large
}

// //////////////////////////////////////////////////
// ///      PUBLIC APIs     ////////////////////////
IQSwitchSimulator::~IQSwitchSimulator() {
  // de-constructor traffic related objects
  // std::cerr << "destory traffic pattern ...\n";
  for (auto &traf: _traffic_patterns) {
    if (traf.second) {
      delete (traf.second);
      traf.second = nullptr;
    }
  }
  // std::cerr << "destory injection ...\n";
  for (auto &inj : _injections) {
    if (inj.second) {
      delete (inj.second);
      inj.second = nullptr;
    }
  }
  //
  // std::cerr << "destory switch ...\n";
  _switch->reset();
  if (_switch)
    delete (_switch);
  // uninstall channels
  // std::cerr << "destory channels ...\n";
  clear_channels();

  // delete statistics
  // std::cerr << "destory stats ...\n";
  for(auto& ins : _instruments) {
    if(ins.first != "throughput") delete(ins.second);
  }
//  if (_delay_stats)
//    delete (_delay_stats);
//  if (_queue_length_stats)
//    delete (_queue_length_stats);
}
void IQSwitchSimulator::simulate() {
  assert(_channel_installed && "Channels MUST be installed before simulate");
  if (!_reset)
    reset();
  bool first_call = true;
  const double thres_throughput = 0.999;
  std::map<std::string, double> stable_region_boundary ;
  for (auto &inj : _injections) {
    _current_load = inj.first.first;
    _current_burst_size = inj.first.second;
    InjectionModel *injection = inj.second;

    for (auto &traf : _traffic_patterns) {
      TrafficPattern *traffic = traf.second;
      if (stable_region_boundary.count(traffic->name()) && stable_region_boundary[traffic->name()] <= _current_load){
        // skip it, as current load is of our boundary
      } else {
       // if (!simulate_on(injection, traffic, first_call)) {// unstable
        simulate_on(injection, traffic, first_call);
        if (_stats_results[traffic->name()]["throughput"].back() < thres_throughput)
          stable_region_boundary[traffic->name()] = _current_load;
        //}
      }
      // call reset
      IQSwitchSimulator::reset();
      first_call = false;
    }
  }
}

void IQSwitchSimulator::reset() {
  if (_reset)
    return;
  // The following two steps might be unnecessary
  // since they might be done by the switch reset.
  for (auto &ic : _input_channels) {
    while (!ic->empty()) {
      Packet *pkt = ic->receive();
      delete (pkt);
    }
  }
  for (auto &oc : _output_channels) {
    while (!oc->empty()) {
      Packet *pkt = oc->receive();
      delete (pkt);
    }
  }
  _switch->reset();
  // reset  traffic related objects
  for (auto &inj : _injections) {
    inj.second->reset();
  }
  for (auto &traf : _traffic_patterns) {
    traf.second->reset();
  }
  // reset all instruments
  for(auto &ins : _instruments)
    if(ins.first != "throughput") ins.second->reset();
  _reset = true;
}
void IQSwitchSimulator::display_stats(std::ostream &os) const {
  std::string s = _stats_results.dump(4);
  os << s << "\n";
}
} // namespace saber
