//  switch.h header file

#ifndef SWITCH_H
#define SWITCH_H

#include <iostream>
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <exception>

#include <nlohmann/json.hpp>

#include <packet.hpp>
#include <channel.hpp>
#include <timer.hpp>
#include <exceptions.hpp>


#include <traffic_generator/traffic_pattern_factory.h>
#include <traffic_generator/injection_model_factory.h>
#include <scheduler/scheduler_factory.h>

namespace saber {
// Base class for various switches
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
class Switch {
 protected:
  static int ID; // class id
  const std::string _name; // switch name
  const int _id; // id for each object (identity for each object, reserved for future use)

  int _num_inputs; // # of inputs
  int _num_outputs; // # of outputs

  unsigned _delay{0};// length of each switching cycle (default: ignoring transmission delay)
  bool _channel_installed{false}; // indicator variable for channel installation

  // input channels: channels connecting to each input port, allowing the inputs to read packets from traffic generator
  std::vector<Channel<Packet> *> _input_channel;
  // output channels: channels connecting to each output port, allowing packets to depart from the switch
  // In this project, it is an interface that allows the instrument tools to do instruments on each packet departing
  // from the switch.
  std::vector<Channel<Packet> *> _output_channel;

/** @brief Hidden constructor for class Switch
 *
 * @param name                  String Name of the switch object (cannot be empty)
 * @param num_inputs            Integer Number of inputs (should be a positive integer)
 * @param num_outputs           Integer Number of outputs (should be a positive integer)
 */
  Switch(std::string name, int num_inputs, int num_outputs);
  /** @brief Read inputs
   *
   * Read packets arriving at input ports of the switch.
   *
   */
  virtual void read_inputs() = 0;
  /** @brief Write outputs
   *
   * Departing packets from each output.
   *
   */
  virtual void write_outputs() = 0;

 public:
  /** @brief Channel Installation
   *
   * Installing input channels and output channels, so that it can read inputs from input channels and write outputs
   * by output channels.
   *
   * @param inputs         Reference to the vector of input channels
   * @param outputs        Referecne to the vector of output channels
   */
  void install_channels(const std::vector<Channel<Packet> *> &inputs, const std::vector<Channel<Packet> *> &outputs);
  virtual ~Switch() = default;
  /** @brief Reset the switch
   *
   * Soft-reboot the switch, resetting all the internal parameters into their default initial values.
   */
  virtual void reset() { }
  /** @brief Display Parameters/States of the switch
   *
   * @param os      Reference to std::ostream
   */
  virtual void display(std::ostream &os) const;
  /** @brief Do the switching
   *
   * This is the major API for the switch. Generally, it does all the work the switch should do within a switching
   * cycle (_e.g.,_ handling arrivals, calculate the schedule, and handling the departures)
   *
   */
  virtual void switching() = 0;
  /** @brief Replace the internal scheduler
   *
   * This API allows the switch to switch from one scheduler to another without
   * re-creating a new switch object. It tries to simulate some real-life switches
   * that support hot-plug scheduling modules.
   *
   * @param conf    Refernce to a json object, configuration of the new scheduler.
   */
  virtual void replace_scheduler(const json& conf) = 0;
}; // class Switch

} // namespace saber

#endif // SWITCH_H
