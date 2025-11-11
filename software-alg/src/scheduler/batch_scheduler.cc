//

#include "batch_scheduler.h"
#include <switch/iq_switch.h>

namespace saber {
// Implementation of class BatchScheduler
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////
BatchScheduler::BatchScheduler(const std::string &name, int num_inputs, int num_outputs, int frame_size, bool frame_size_fixed):
    Scheduler(name, num_inputs, num_outputs, false),
    _frame_size_fixed(frame_size_fixed),
    _frame_size_init(static_cast<const size_t>(frame_size)),
    _frame_size(static_cast<size_t>(frame_size)),
    _cf_rel_time(0),
    _schedules(static_cast<unsigned long>(frame_size), std::vector<int>(num_inputs, -1)),
    _schedules_pre{},
    _pf_rel_time(0)
{ }

void BatchScheduler::reset() {
  _frame_size = _frame_size_init;
  _cf_rel_time = 0;
  _schedules.resize(_frame_size, std::vector<int>(_num_inputs, -1));
  _schedules_pre.clear();
  _pf_rel_time = 0;
}
void BatchScheduler::display(std::ostream &os) const {
  Scheduler::display(os);
  os << "---------------------------------------------------------------------------------\n";
  os << "frame_size       : " << frame_size();
  os << "\nframe size fixed : " << frame_size_fixed();
  os << "\nschedules (previous frame): \n";
  int cnt = 1;
  for ( const auto& m : _schedules_pre ) os << "Time Slot " << (cnt ++) << " : " << m << "\n";
  os << "\nschedules (current frame): \n";
  for ( const auto& m : _schedules ) os << "Time Slot " << (cnt ++) << " : " << m << "\n";
}
} // namespace saber


