//
#include "switch.h"

namespace saber {
// Implementation of class Switch
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
int Switch::ID = 0;
Switch::Switch(std::string name, int num_inputs, int num_outputs) :
    _name(std::move(name)), _id(ID++), _num_inputs(num_inputs), _num_outputs(num_outputs) {
}
void Switch::install_channels(const std::vector<Channel<Packet> *> &inputs,
                              const std::vector<Channel<Packet> *> &outputs)  {
  assert(inputs.size() == _num_inputs);
  assert(outputs.size() == _num_outputs);
  assert(std::all_of(inputs.cbegin(), inputs.cend(), [](Channel<Packet> *ch) { return ch != nullptr; }));
  assert(std::all_of(outputs.cbegin(), outputs.cend(), [](Channel<Packet> *ch) { return ch != nullptr; }));
  _input_channel.clear();
  _input_channel.insert(_input_channel.begin(), inputs.begin(), inputs.end());
  _output_channel.clear();
  _output_channel.insert(_output_channel.begin(), outputs.begin(), outputs.end());
  _channel_installed = true;
}
void Switch::display(std::ostream &os) const  {
  os <<   "name               : " << _name
     << "\nid                 : " << _id
     << "\nnumber of inputs   : " << _num_inputs
     << "\nnumber of outputs  : " << _num_outputs
     << "\ndelay              : " << _delay
     << "\nchannel installed  : " << _channel_installed
     << "\n";
}

} // namespace saber
