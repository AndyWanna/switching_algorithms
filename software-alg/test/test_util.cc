// test_util.cc

// main() provided by Catch in the file test_main.cc
//

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <chrono> // for std::chrono::system_clock::now()
#include <sstream>
#include <random>

#include <stats.hpp>
#include <timer.hpp>
#include <channel.hpp>

using namespace saber;
using json = nlohmann::json;

// Test cases
TEST_CASE("1.1. Stats Constructor/New should work correctly", "[stats]") {
    json conf = {
            {"name", "general"}
    };

    SECTION("Create a simplest stats object") {
        auto *s = Stats::New(conf);
        REQUIRE(s != nullptr);
        delete(s);
    }
}

TEST_CASE("1.2. Stats simple use case", "[stats]") {
    json conf = {
            {"name", "general"}
    };
    auto *s = Stats::New(conf);
    SECTION("Test on Poisson r.v.") {
        std::random_device rd;
        std::mt19937 gen(rd());
        double mean = 4.0;
        std::poisson_distribution<> d(mean);
        int t = int(1e6);
        for(int i = 0;i < t;++ i) s->add_sample(d(gen));
        CHECK(s->num_samples() == t);
        CHECK(s->average() == Approx(4.0).margin(0.4));
        CHECK(s->variance() == Approx(4.0).margin(0.4));
    }
    delete(s);
}

#ifdef  ENABLE_HDRHISTOGRAM
TEST_CASE("1.3. Stats histogram use case", "[stats]") {
    json conf = {
            {"name", "general"},
            {"enable_histogram", true}
    };
    auto *s = Stats::New(conf);
    SECTION("New should succeed") {
        REQUIRE(s != nullptr);
    }
    SECTION("Simple histogram") {
        s->add_sample(1);
        s->add_sample(2);
        CHECK(s->num_samples() == 2);
        CHECK(s->max() == 2);
        CHECK(s->min() == 1);
        CHECK(s->average() == Approx(1.5));
        CHECK(s->get_hdr_mean() == Approx(1.5));
        CHECK(s->get_hdr_max() == 2);
        CHECK(s->get_hdr_min() == 1);

    }
    delete(s);
}
#endif

TEST_CASE("2. Timer create/simple usage tests", "[timer]") {
    json conf = {
            {"name", "my_timer"}
    };
    auto *t = Timer::New(conf);

    SECTION("Constructor/New should work correctly") {
        REQUIRE(t != nullptr);
    }

    SECTION("Initial time should be zero") {
        CHECK(t->now() == 0);
    }

    SECTION("Update should work") {
        t->update();
        CHECK(t->now() == 1);
        t->update(10);
        CHECK(t->now() == 11);
    }

    delete(t);
}

TEST_CASE("3. Channel create/operate tests", "[channel]") {
    json conf = {
            {"name", "test"}
    };

    auto *c = Channel<int>::New(conf);

    SECTION("New should succeed") {
        REQUIRE(c != nullptr);
    }
    SECTION("Send should work") {
        int data = 100;
        c->send(&data);
        CHECK(c->size() == 1);// buffer size should be 1 now
        auto *d = c->receive();
        CHECK(c->size() == 0);
        CHECK(*d == data);
    }
    SECTION("Multiple sends/receives") {
        std::vector<int> nums(100, 0);
        for(int& d : nums){
            c->send(&d);
        }
        CHECK(c->size() == nums.size());
        for(int d : nums) {
            auto *pd = c->receive();
            CHECK(*pd == d);
        }
        CHECK(c->size() == 0);
    }
    SECTION("Empty channel cannot receive") {
        CHECK(c->empty());
        auto* pd = c->receive();
        CHECK(pd == nullptr);
    }
    delete (c);
}
