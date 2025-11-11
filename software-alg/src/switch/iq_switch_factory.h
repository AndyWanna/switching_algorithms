//  iq_switch_factory.h header file

#ifndef IQ_SWITCH_FACTORY_H
#define IQ_SWITCH_FACTORY_H

#include "iq_switch.h"

namespace saber {
// Class for input-queued switch factory
class IQSwitchFactory {
 public:
 static IQSwitch* Create(const json &switch_conf, const json &scheduler_conf={});
};
}

#endif // IQ_SWITCH_FACTORY_H
