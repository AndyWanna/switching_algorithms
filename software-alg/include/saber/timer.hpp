// timer.h header file
#ifndef  TIMER_H
#define  TIMER_H

#include <string>
#include <cassert>
#include <utility>

#include <utils.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;
namespace saber {
    //! class Timer
    class Timer {
    public:
        //! Get current time
        size_t get_time() const {
            return _time;
        }
        size_t now() const {
            return _time;
        }
        //! Update time
        void update(size_t increment = 1) {
            _time += increment;
        }
        //! Reset timer
        void reset() {
            _time = 0;
        }
        //! Get timer's name
        std::string get_name() const {
            return _name;
        }
        std::string name() const {
            return _name;
        }
        //! Display timer's internal
        virtual void display(std::ostream &os) const {
            os << "name : " << _name
               << "\ntime : " << _time
               << "\n";
        }
        static Timer *New(const json &conf) {
            assert (conf.count("name") && "Argument conf MUST contain at least name.");
            std::string name = conf["name"];
            assert(!name.empty() && "Argument name cannot be empty!");
            Timer *t = new Timer(name);
            return t;
        }

    protected:
        explicit Timer(std::string name) : _name(std::move(name)), _time(0) {
        }
        const std::string _name;
        size_t _time;
    }; // class Timer
} // namespace saber
#endif // TIMER_H
