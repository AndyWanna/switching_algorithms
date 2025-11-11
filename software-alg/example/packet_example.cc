// Examples for dealing with packet

#include <iostream>
#include <queue>

#include <packet.hpp>
#include <utils.hpp> // operator<< for std::queue
#include "example_macros.h"

using namespace std;
using namespace saber;

EXAMPLE_BEGIN;

  EXAMPLE("1. Construct a packet with default parameters")
  {
    Packet p;
    std::cout << p << "\n";
  }

  EXAMPLE("2. Construct a packet with source & destination")
  {
    Packet p(1, 2);
    std::cout << p << "\n";
  }

  EXAMPLE("3. Construct a packet with 3 parameters")
  {
    Packet p(1, 3, 9);
    std::cout << p << "\n";
  }

  EXAMPLE("4. Construct a packet with 4 parameters")
  {
    Packet p(1, 3, 9, 90);
    std::cout << p << "\n";
  }

  EXAMPLE("5. Deal with a queue with packet pointers")
  {
    std::queue<Packet *> my_queue;
    size_t t = 0;
    int n = 8;
    for (int s = 0; s < n; ++s)
      for (int d = s + 1; d < n; ++d) {
        my_queue.push(new Packet(s, d, t));
        t += 2;
      }
    std::cout << my_queue << "\n\n";
    while (!my_queue.empty()) {
      auto *p = my_queue.front();
      my_queue.pop();
      if (p != nullptr) delete (p);
    }
  }
EXAMPLE_END;

int main() {
  EXAMPLE_RUN;
  return 0;
}