// batch_scheduler.h header file

#ifndef BATCH_SCHEDULER_H
#define BATCH_SCHEDULER_H

#include "scheduler.h"

namespace saber {
// Macros for batch-related parameters
#ifndef MAX_FRAME_SIZE
#define MAX_FRAME_SIZE 32768
#endif
#ifndef DEF_FRAME_SIZE
#define DEF_FRAME_SIZE 4096
#endif
#ifndef FRAME_SIZE_BLOCK
#define FRAME_SIZE_BLOCK 128
#endif

// Base class BatchScheduler
// //////////////////////////////////////////////////////////////////////////////////////////////////////
class BatchScheduler : public Scheduler {
  friend class SchedulerFactory;
 protected:
  const size_t _frame_size_init; // initial value for frame size
  const bool _frame_size_fixed; // whether or not to allow frame size change after initialization
  size_t _frame_size; // current frame size
  size_t _cf_rel_time; // current frame relative time
  std::vector<std::vector<int> > _schedules; // schedules for current frame
  std::vector<std::vector<int> > _schedules_pre;// schedules for previous frame
  int _pf_rel_time; // previous frame relative time

  // hidden constructor
  BatchScheduler(const std::string &name, int num_inputs, int num_outputs, int frame_size, bool frame_size_fixed) ;
 public:
  ~BatchScheduler() override = default;
  void schedule(const IQSwitch *sw) override = 0;
  void init(const IQSwitch *sw) override = 0;
  size_t frame_size() const { return _frame_size; }
  bool frame_size_fixed() const { return _frame_size_fixed; }
  void reset() override ;

  void display(std::ostream &os)  const override ;

  //! reserved for future extensions
  void dump_stats(std::ostream &os) override {}
};

} // namespace saber

#endif // BATCH_SCHEDULER_H
