// packet.h header file

#ifndef PACKET_H
#define PACKET_H

#include <iostream>

namespace saber {
// Class for fake packet
// ///////////////////////////////////////////////////////////////////////////////////////
struct Packet {
  size_t _arrival_time{0};
  size_t _departure_time{0};

  int _source{-1};
  int _destination{-1};

  Packet(int source, int destination, size_t arrival_time, size_t departure_time) :
      _arrival_time(arrival_time), _departure_time(departure_time),
      _source(source), _destination(destination) {

  }
  Packet(int source, int destination, size_t arrival_time) :
      Packet(source, destination, arrival_time, 0) {

  }
  Packet(int source, int destination) : Packet(source, destination, 0, 0) {}
  Packet() : Packet(-1, -1, 0, 0) {}

  void set_arrival_time(size_t arrival_time) {
    _arrival_time = arrival_time;
  }
  void set_departure_time(size_t departure_time) {
    _departure_time = departure_time;
  }
  void set_source(int source) {
    _source = source;
  }
  void set_destination(int destination) {
    _destination = destination;
  }
  size_t get_arrival_time() const {
    return _arrival_time;
  }
  size_t get_departure_time() const {
    return _departure_time;
  }
  int get_source() const {
    return _source;
  }
  int get_destination() const {
    return _destination;
  };
  size_t arrival_time() const {
    return _arrival_time;
  }
  size_t departure_time() const {
    return _departure_time;
  }
  int source() const {
    return _source;
  }
  int destination() const {
    return _destination;
  }
  std::ostream& operator<<(std::ostream& os) const {
    os  << "arrival time     : "   << arrival_time()
        << "\ndeparture time   : " << departure_time()
        << "\nsource           : " << source()
        << "\ndestination      : " << destination();
    return os;
  }
}; // Packet
} // namespace saber

// Make std::cout << work for Packet objects
inline std::ostream &operator<<(std::ostream &os, const saber::Packet &pkt) {
  pkt.operator<<(os);
  return os;
}

#endif // SSCHEDSIMULATOR_PACKET_H
