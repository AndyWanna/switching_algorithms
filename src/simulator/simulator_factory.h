//  simulator_factory.h header file

#ifndef SIMULATOR_FACTORY_H
#define SIMULATOR_FACTORY_H

#include <exceptions.hpp>
#include "simulator.h"

namespace saber {
class SimulatorFactory {
 public:
  static Simulator *Create(const json &conf) ;
}; // class IQSwitchSimulator
} // namespace saber

#endif // SIMULATOR_FACTORY_H
