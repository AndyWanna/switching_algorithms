// test_scheduler.cc

// main() provided by Catch in the file test_main.cc
//

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>
#include <random>

#include <scheduler/scheduler_factory.h>
#include <switch/iq_switch_factory.h>

using namespace saber;
using json = nlohmann::json;


TEST_CASE("1. Creating scheduler should work", "[scheduler]") {
  int n = 4;
  json sched_conf = {
      {"name", "maximum_weight"},
      {"num_inputs", n},
      {"num_outputs", n}
  };

  SECTION("MWM") {
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Randomized Maximal") {
    sched_conf["name"] = "randomized_maximal";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Round Robin") {
    sched_conf["name"] = "round_robin";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("iSLIP") {
    sched_conf["name"] = "islip";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Single-iteration LQF") {
    sched_conf["name"] = "slqf";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Ierative Power of (Two) Choices") {
    sched_conf["name"] = "ipoc";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Queue Proportional Sampling ") {
    sched_conf["name"] = "qps";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Small-Batch Queue Proportional Sampling ") {
    sched_conf["name"] = "sb_qps";
    auto *sched = SchedulerFactory::Create(sched_conf);
    REQUIRE(sched != nullptr);
    delete(sched);
  }
  SECTION("Unexpected Parameters") {
    sched_conf["name"] = "";
    REQUIRE_THROWS(SchedulerFactory::Create(sched_conf));
    sched_conf["name"] = "not_exist";
    REQUIRE_THROWS(SchedulerFactory::Create(sched_conf));
  }
}

TEST_CASE("2.1. Some simple cases for some schedulers", "[scheduler]") {
  int n = 4;
  json sched_conf = {
      {"name", "maximum_weight"},
      {"num_inputs", n},
      {"num_outputs", n}
  };
  // First, we need a dummy switch
  std::vector<std::vector<size_t> > initial_queue = {
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1}
  };
  json sw_conf = {
      {"type", "dummy"},
      {"name", "test_switch"},
      {"num_inputs", n},
      {"num_outputs", n},
      {"initial_queue_length", initial_queue}
  };
  auto* sw = IQSwitchFactory::Create(sw_conf);
  REQUIRE(sw != nullptr);

  SECTION("MWM") {
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == i);
    delete(sched);
  }
  SECTION("Randomized Maximal") {
    sched_conf["name"] = "randomized_maximal";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == i);
    delete(sched);
  }

  SECTION("iSLIP") {
    sched_conf["name"] = "islip";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == i);
    delete(sched);
  }
  SECTION("Single-iteration LQF") {
    sched_conf["name"] = "slqf";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == i);
    delete(sched);
  }
  SECTION("Ierative Power of (Two) Choices") {
    sched_conf["name"] = "ipoc";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == i);
    delete(sched);
  }
  SECTION("Queue Proportional Sampling ") {
    sched_conf["name"] = "qps";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == i);
    delete(sched);
  }
  delete(sw);
}

TEST_CASE("2.2. A little bit more complicated cases for some schedulers", "[scheduler]") {
  int n = 4;
  json sched_conf = {
      {"name", "maximum_weight"},
      {"num_inputs", n},
      {"num_outputs", n}
  };
  // First, we need a dummy switch
  std::vector<std::vector<size_t> > initial_queue = {
      {0, 0, 0, 1},
      {0, 0, 10, 0},
      {0, 90, 0, 0},
      {8, 0, 0, 0}
  };
  json sw_conf = {
      {"type", "dummy"},
      {"name", "test_switch"},
      {"num_inputs", n},
      {"num_outputs", n},
      {"initial_queue_length", initial_queue}
  };
  auto* sw = IQSwitchFactory::Create(sw_conf);
  REQUIRE(sw != nullptr);

  SECTION("MWM") {
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == (n - i  - 1));
    delete(sched);
  }
  SECTION("Randomized Maximal") {
    sched_conf["name"] = "randomized_maximal";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == (n - i  - 1));
    delete(sched);
  }

  SECTION("iSLIP") {
    sched_conf["name"] = "islip";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == (n - i  - 1));
    delete(sched);
  }
  SECTION("Single-iteration LQF") {
    sched_conf["name"] = "slqf";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == (n - i  - 1));
    delete(sched);
  }
  SECTION("Ierative Power of (Two) Choices") {
    sched_conf["name"] = "ipoc";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == (n - i  - 1));
    delete(sched);
  }
  SECTION("Queue Proportional Sampling ") {
    sched_conf["name"] = "qps";
    auto *sched = SchedulerFactory::Create(sched_conf);
    sched->init(sw); // init MUST be called if initial queue is not all zeros
    sched->schedule(sw);
    for(int i = 0;i < n;++ i) CHECK(sched->match_with(i) == (n - i  - 1));
    delete(sched);
  }
  delete(sw);
}