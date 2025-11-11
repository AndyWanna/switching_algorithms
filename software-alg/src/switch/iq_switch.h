// iq_switch.h header file

#ifndef IQ_SWITCH_H
#define IQ_SWITCH_H

#include "switch.h"

namespace saber {
class IQSwitchFactory;
// Base class for input-queued switches
// This base class is for switches that buffer all packets at the input side.
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IQSwitch : public Switch {
  friend class IQSwitchFactory;
 public:
  // Virtual Output Queue matrix type
  using voq_mat_t = std::vector<std::vector<std::deque<Packet *> > >;
  // Queue length matrix type
  using queue_len_mat_t = std::vector<std::vector<size_t> >;
  // vector of pairs of source & destination
  using sd_pair_vec_t = std::vector<std::pair<int/*src*/, int/*dst*/> >;
 protected:
  // Arrivals (using to temporarily store "packets" reading from the input channels and telling the scheduler
  // about the arrivals if necessary). Arrivals a format as a vector of (src, dst).
  sd_pair_vec_t _arrivals{};
  // Indicator variable: whether to enable departure information
  const bool _enable_departure_info{false};
  // Departures:
  sd_pair_vec_t _departures{};
  // Pointer to a scheduler
  Scheduler *_schedule{nullptr};
  // Pointer to a timer
  Timer *_timer{nullptr};

  IQSwitch(std::string name, int num_inputs, int num_outputs, bool enable_departure_info) : Switch(std::move(name), num_inputs, num_outputs),
                                                                                            _enable_departure_info(enable_departure_info){
    _arrivals.reserve(std::min(_num_inputs, _num_outputs)); // reserve space for arrivals
    if(_enable_departure_info) _departures.reserve(std::min(_num_inputs, _num_outputs)); // reserve space
  }
  void read_inputs() override = 0 ;
  void write_outputs() override = 0;

 public:
  // static IQSwitch *New(const json &switch_conf, const json &scheduler_conf) ;
  ~IQSwitch() override {
#ifdef DEBUG
    std::cout << "DEBUG: IQSwitch()::~IQSwitch()"
              << " is called\n";
#endif
    if(_timer != nullptr) delete(_timer);
    if(_schedule != nullptr) delete(_schedule);
  }
  void reset() override {}
  void display(std::ostream &os) const override { Switch::display(os); }
  void switching() override = 0;
  void replace_scheduler(const json& conf) override {
      Scheduler *new_sched = SchedulerFactory::Create(conf);
      assert (new_sched != nullptr);
      delete (_schedule);
      _schedule = new_sched;
  }

  // More virtual APIs for scheduler
  // // The following two APIs seem to be not very suitable or even necessary (removed @01-01-2019)
  /*
  virtual const voq_mat_t& get_queue_matrix() const = 0;
  virtual const queue_len_mat_t& get_queue_length_matrix() const = 0;
   */
  // Get the number of packets that want to go from input port source ti output port destination
  virtual size_t get_queue_length(int source, int destination) const = 0;
  // Get the arrivals
  virtual const std::vector<std::pair<int/* source */, int /* destination */> > &get_arrivals() const { return _arrivals; };
  // Get the arrival summary
  virtual const sd_pair_vec_t &get_departures() const {
    if(!_enable_departure_info) throw DisabledMemberException("Member departures is disabled.");
    return _departures;
  }
  // Get the scheduler
  virtual const Scheduler *const get_scheduler() const { return _schedule; }
  // Get the timer
  virtual const Timer *const get_timer() const { return _timer; }
};
// Class for dummy inqut-queued switch (for test purpose)
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DummyIQSwitch : public IQSwitch {
  friend class IQSwitchFactory;
 protected:
  // Queue length matrix
  queue_len_mat_t _queue_len_mat;

  DummyIQSwitch(std::string name, int num_inputs, int num_outputs, const queue_len_mat_t& initial_queue_len) :
      IQSwitch(std::move(name), num_inputs, num_outputs, false), _queue_len_mat(initial_queue_len){
    assert(initial_queue_len.size() == num_inputs && "initial_queue_len should be valid");
    for(const auto& row : initial_queue_len) assert(row.size() == num_outputs && "initial_queue_len should be valid");
  }

  void read_inputs() override  { /* do nothing */ };
  void write_outputs() override { /* do nothing */ };

 public:
  ~DummyIQSwitch() override = default;

  void reset() override {
    for(auto& row_queue_len : _queue_len_mat) std::fill(row_queue_len.begin(), row_queue_len.end(), 0);
  }

  void display(std::ostream &os) const override {
    IQSwitch::display(os);
    os << "------------------------------------------------------------------------------\n";
    os << "queue length matrix: \n";
    for(auto& row_queue_len : _queue_len_mat) {
      std::cout << row_queue_len;
      std::cout << "\n";
    }
  }
  void switching() override { /* do nothing */ }
  void replace_scheduler(const json& conf) override { /* do nothing */ }

  size_t get_queue_length(int source, int destination) const override {
    assert(source >= 0 && source < _num_inputs && "Source should be valid");
    assert(destination >= 0 && destination < _num_outputs && "Destination should be valid");
    return _queue_len_mat[source][destination];
  };
  const Scheduler *const get_scheduler() const override { return nullptr; }
  const Timer *const get_timer() const override { return nullptr; }
}; // class DummyIQSwitch

// Class for (simplified) Input-Queued Switch
// This class is motivated by some crossbar schedulers that have very poor delay performances or even can not be stable
// when the offered load is high enough (still admissible). In such cases, if we still buffer all the packets, then
// the memory of our PC might be sooner-or-later "eaten up". Therefore, we provide such a class for input-queued switches
// which only record how many packets are there in each VOQ instead of buffering all packets. Of course, the dark side is
// now you can not get the delay of each packet. However, if the system is still stable, then we can still get the average
// packet delay by using Little's Law.
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class sIQSwitch : public IQSwitch {
  friend class IQSwitchFactory;
 protected:
  queue_len_mat_t _queue_len_mat;

  sIQSwitch(std::string name, int num_inputs, int num_outputs, const json &conf) ;
  sIQSwitch(std::string name, int num_inputs, int num_outputs, const queue_len_mat_t& initial_queue_len, const json &conf) ;

  void read_inputs() override ;
  void write_outputs() override ;
  void clear();
 public:
  void display(std::ostream &os) const override ;
  ~sIQSwitch() override = default;
  void reset() override ;

  void switching() override ;
  size_t get_queue_length(int source, int destination) const override ;
  const Scheduler *const get_scheduler() const override { return _schedule; }
  const Timer *const get_timer() const override { return _timer; }
};
// Class for (generic) Input-Queued Switch
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class gIQSwitch : public IQSwitch {
  friend class IQSwitchFactory;
 protected:
  using voq_t=std::vector<std::vector<std::deque<Packet *> > >;
  voq_t _virtual_output_queue;

  gIQSwitch(std::string name, int num_inputs, int num_outputs, bool enable_arrival_summary, const json &conf) ;
  void init_timer();
  void read_inputs() override ;
  void write_outputs() override ;
  void _write_outputs_depart_info_enabled();
  void _write_outputs_depart_info_disabled();

 public:
  void display(std::ostream &os) const override ;
  ~gIQSwitch() override = default;
  void reset() override ;
  // Clear all packets residing in queues & channels
  void clear() ;
  void switching() override ;

  size_t get_queue_length(int source, int destination) const override;
  const Scheduler *const get_scheduler() const override;
  const Timer *const get_timer() const override;
}; // class gIQSwitch
} // namespace saber

#endif // IQ_SWITCH_H
