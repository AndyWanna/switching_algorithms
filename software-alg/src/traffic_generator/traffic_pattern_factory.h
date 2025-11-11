// traffic_pattern_factory.h header file

#ifndef TRAFFIC_PATTERN_FACTORY_H
#define TRAFFIC_PATTERN_FACTORY_H

#include <iostream>

#include "traffic_pattern.h"
#include "uniform_traffic_pattern.h"
#include "log_diagonal_traffic_pattern.h"
#include "quasi_diagonal_traffic_pattern.h"
#include "diagonal_traffic_pattern.h"

#include <nlohmann/json.hpp>

namespace saber {
using json=nlohmann::json;
class TrafficPatternFactory {
 public:
  static TrafficPattern *Create(const json &conf);
};

} // namespace saber
#endif // TRAFFIC_PATTERN_FACTORY_H
