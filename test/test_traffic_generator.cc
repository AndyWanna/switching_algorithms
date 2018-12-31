// test_traffic_generator.cc

// main() provided by Catch in the file test_main.cc
//

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>


#include <traffic_generator/injection_model_factory.h>
#include <traffic_generator/traffic_pattern_factory.h>

using namespace std;
using namespace saber;
using json = nlohmann::json;


// Test Injection Model
TEST_CASE("1. Injection model Constructor/Create should work correctly", "[injection]") {
    json inj_conf = {
        {"name", "bernoulli"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5}
    };

    SECTION( "Valid arguments should construct successfully") {
        InjectionModel *bern_inj = InjectionModelFactory::Create(inj_conf);
        REQUIRE( bern_inj != nullptr );
        delete (bern_inj);
        inj_conf["name"] = "onoff";
        inj_conf["burst_size_avg"] =  2.0;
        auto *onoff_inj = InjectionModelFactory::Create(inj_conf);
        REQUIRE( onoff_inj != nullptr );
        delete (onoff_inj);
        inj_conf["name"] = "extended-onoff";
        auto *exonoff_inj = InjectionModelFactory::Create(inj_conf);
        REQUIRE( exonoff_inj );
        delete (exonoff_inj);
    }

    SECTION( "Invalid name should not construct") {
        inj_conf["name"] = "not_exist";
        InjectionModel *bern_inj = InjectionModelFactory::Create(inj_conf);
        REQUIRE( bern_inj == nullptr );
    }


}

TEST_CASE("2. Bernoulli Injection should work as expected", "[injection]") {
    json inj_conf = {
        {"name", "bernoulli"},
        {"num_nodes", 10},
        {"seed", 0},
        {"rate", 0.5}
    };

    SECTION( "How about injecting with rate 0.5 (may fail)") {
        InjectionModel *bern_inj = InjectionModelFactory::Create(inj_conf);
        double c = 0;
        double T = 1e6;
        for(int i = 0;i < T;++ i) if(bern_inj->inject(0) == InjectionState::ON) c += 1.0;
        c /= T;
        REQUIRE( c == Approx(0.5).margin(0.1));
        delete (bern_inj);
    }
    SECTION( "How about injecting with rate 0.8 (may fail)") {
        inj_conf["rate"] = 0.8;
        InjectionModel *bern_inj = InjectionModelFactory::Create(inj_conf);
        double c = 0;
        double T = 1e6;
        for(int i = 0;i < T;++ i) if(bern_inj->inject(0) == InjectionState::ON) c += 1.0;
        c /= T;
        REQUIRE( c == Approx(0.8).margin(0.1));
        delete (bern_inj);
    }
    SECTION( "How about injecting with rate 1.0") {
        inj_conf["rate"] = 1.0;
        InjectionModel *bern_inj = InjectionModelFactory::Create(inj_conf);
        double c = 0;
        double T = 1e6;
        for(int i = 0;i < T;++ i) if(bern_inj->inject(0) == InjectionState::ON) c += 1.0;
        c /= T;
        REQUIRE( c == Approx(1.0));
        delete (bern_inj);
      }
}


// Test Traffic Pattern
TEST_CASE("Constructor/Create should work", "[traffic-pattern]") {
    auto seed = static_cast<unsigned int>( std::chrono::system_clock::now().time_since_epoch().count());
    auto num_nodes = 8;
    json conf = {
        {"name", "uniform"},
        {"num_nodes", num_nodes},
        {"seed", seed}
    };
    
    SECTION("Correct arguments should construct successfully") {
        auto *uniform_tp = TrafficPatternFactory::Create(conf);
        REQUIRE( uniform_tp != nullptr );
        delete (uniform_tp);
        conf["name"] = "log_diagonal";
        auto *log_d_tp = TrafficPatternFactory::Create(conf);
        REQUIRE ( log_d_tp != nullptr );
        delete ( log_d_tp );
        conf["name"] = "quasi_diagonal";
        auto *quasi_d_tp = TrafficPatternFactory::Create(conf);
        REQUIRE (quasi_d_tp != nullptr);
        delete ( quasi_d_tp );
        conf["name"] = "diagonal";
        auto *d_tp = TrafficPatternFactory::Create(conf);
        REQUIRE (d_tp != nullptr );
        delete (d_tp );
    }
    SECTION("Invalid name should not construct") {
        conf["name"] = "hehe";
        auto *hehe = TrafficPatternFactory::Create(conf);
        REQUIRE( hehe == nullptr );
    }
}
