// scheduler.h header file

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include <algorithm>    // std::find
#include <cassert>
#include <iostream>
#include <chrono>
#include <limits>
#include <random>

#include <bitset>

#include <utils.hpp>
#include <nlohmann/json.hpp>

namespace saber {
using json = nlohmann::json;

#ifndef MAX_PORTS
#define MAX_PORTS 2048
#endif

enum class PortType {
  IN = 0, OUT = 1
};

// Class declarations
class SchedulerFactory;
class Scheduler;
class IQSwitch;

// Base class for scheduler
// /////////////////////////////////////////////////////////////////////////////////////////////////////
class Scheduler {
  friend class SchedulerFactory;
 protected:
  static int ID;// id
  const std::string _name;// name of the core
  const int _id;

  int _num_inputs; // # of inputs
  int _num_outputs;// # of outputs

  const bool _out_match_enabled;// whether or not to enable out match
  std::vector<int> _in_match;
  std::vector<int> _out_match;
  // Hidden constructor
  Scheduler(std::string name, int num_inputs, int num_outputs, bool out_match_enabled);
 public:
  std::string name() const { return _name; }
  int id() const { return _id; }
  int num_inputs() const { return _num_inputs; }
  int num_outputs() const { return _num_outputs; }

  const std::vector<int> &in_match() const { return _in_match; }
  const std::vector<int> &out_match() const {
    if (!_out_match_enabled) std::cerr << "out match is turned off!\n";
    return _out_match;
  }
  bool out_match_enabled() const { return _out_match_enabled; }

  virtual ~Scheduler() = default;

  // virtual APIs
  virtual void schedule(const IQSwitch *sw) = 0;
  virtual void init(const IQSwitch *sw) = 0;
  virtual void reset() = 0;
  virtual void display(std::ostream &os) const;
  int match_with(int source, const PortType &pt = PortType::IN) const ;

  // reserved for future extensions
  virtual void dump_stats(std::ostream &os) {}

 private:
  // who is matched with source
  int in_match_with(int source) const ;
  // who is matched with dest
  int out_match_with(int dest) const ;
};// class Scheduler

// Class for dummy scheduler (for test purpose)
class DummyScheduler : public Scheduler {
  friend class SchedulerFactory;
 protected:
  DummyScheduler(std::string name, int num_inputs, int num_outputs, bool out_match_enabled) :
  Scheduler(std::move(name), num_inputs, num_outputs, out_match_enabled){}
 public:
  void schedule(const IQSwitch *sw) override { /* do nothing */ }
  void init(const IQSwitch *sw) override { /* do nothing */ }
  void reset() override { /* do nothing */ }
  ~DummyScheduler() override = default;
};

// Base class for randomized schedulers
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
class RandomizedScheduler : public Scheduler {
  friend class SchedulerFactory;
 protected:
  std::mt19937::result_type _seed;
  std::mt19937 _eng{std::random_device{}()};
  RandomizedScheduler(std::string name, int num_inputs, int num_outputs,
                      bool out_match_enabled, std::mt19937::result_type seed);
 public:
  ~RandomizedScheduler() override = default;
  void schedule(const IQSwitch *sw) override = 0;
  void init(const IQSwitch *sw) override = 0;
  void reset() override {}
  void display(std::ostream &os) const override ;
};// class RandomizedScheduler
} // namespace saber

#endif // SCHEDULER_H
