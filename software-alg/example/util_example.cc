//
// This file presents examples for the simple use cases of Timer, Channel, and Stats
// which are defined in include/saber/timer.hpp, include/saber/channel.hpp, and
// include/saber/stats.hpp respectively.

#include <iostream>

#include <timer.hpp>
#include <channel.hpp>
#include <stats.hpp>
#include <utils.hpp> // operator<< for std::queue
#include "example_macros.h"

using namespace std;
using namespace saber;

EXAMPLE_BEGIN;

EXAMPLE("1. Let\'s play with Timer: create & display") {
    json conf = {
            {"name", "test"}
    };
    auto *t = Timer::New(conf);
    t->display(std::cout);
    for(int i = 0;i < 20;++ i) t->update();
    std::cout << "After 20 time units:\n";
    t->display(std::cout);

    delete (t);
}

EXAMPLE("2. Let\'s play with Channel: create & display") {
    json conf = {
            {"name", "test_channel"}
    };

    auto *c = Channel<double>::New(conf);
    double d = 1.0;
    c->display(std::cout);
    while(c->size() < 10) c->send(&d);
    std::cout << "After push 10 units of data: \n";
    c->display(std::cout);
    while(!c->empty()) c->receive();
    delete (c);
}

EXAMPLE("3.1. Let\'s play with Stats: create & display") {
    json conf = {
            {"name", "simple"}
    };
    auto *s = Stats::New(conf);
    s->display(std::cout);
    for(int i = 0;i < 111;++ i) {
        s->add_sample(i % 8) ;
    }
    std::cout << "----------------------------------------------\n";//
    s->display(std::cout);
    delete (s);
}

#ifdef ENABLE_HDRHISTOGRAM
    EXAMPLE("3.2. Let\'s play with Stats: create & display") {
        json conf = {
                {"name", "with_histogram"},
                {"enable_histogram", true}
        };
        auto *s = Stats::New(conf);
        s->display(std::cout);
        for(int i = 0;i < 111;++ i) {
            s->add_sample(i % 8) ;
        }
        std::cout << "----------------------------------------------\n";//
        s->display(std::cout);
        delete (s);
    }
#endif
EXAMPLE_END;


int main(){
    EXAMPLE_RUN;
    return 0;
}

