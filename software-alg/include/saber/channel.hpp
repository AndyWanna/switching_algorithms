// channel.h header file
#ifndef CHANNEL_H
#define CHANNEL_H
#include <queue>

#include <nlohmann/json.hpp>
#include <utils.hpp>

using json = nlohmann::json;
namespace saber {
    // Class for channel
    // ///////////////////////////////////////////////////////////////////////
    template<typename T>
    class Channel {
    protected:
        /** @brief Constructor for class Channel
         *
         *  This constructor creates a channel that is used to connect the switch
         *  and the traffic generator in this project. It is actually a buffer to store data
         *  transmit between them. More precisely, it buffers the data from the traffic
         *  generator to the switch.
         *
         * @param name    String    Name of the channel
         */
        explicit Channel(std::string name) : _id(ID++), _name(std::move(name)) {
            assert (!_name.empty() && "Argument name MUST NOT be empty!");
        }
    public:
        virtual ~Channel() = default;
        /** @brief Send data (to the channel)
         *
         * Push data to (the end of) the buffer queue of the channel
         *
         * @param data      Pointer to data
         */
        virtual void send(T *data) {
            _wait_queue.push(data);
        }
        /** @brief Receive data (from the channel)
         *
         * Pop data from (the front of) the buffer queue of the channel
         *
         * @return  Pointer to data
         */
        virtual T *receive() {
            if (_wait_queue.empty())
                return nullptr;
            T *data = _wait_queue.front();
            _wait_queue.pop();
            return data;
        }
        virtual void display(std::ostream &os) const {
            os <<   "name               : " << _name
               << "\nid                 : " << _id
               << "\n";
            if (!empty()) {
                os << "elements in queue : \n";
                os << _wait_queue;
                os << "\n";
            }
        }
        int size() const {
            return static_cast<int>(_wait_queue.size());
        }
        bool empty() const {
            return _wait_queue.empty();
        }

        static Channel *New(const json &conf) {
            assert (conf.count("name") && "Argument conf MUST contain name.");
            auto name = conf["name"].get<std::string>();

            Channel *result = nullptr;
            if (name.empty()) {
                std::cerr << "Name cannot be empty\n";
            } else {
                result = new Channel(name);
            }
            return result;
        }
    protected:
        static int ID;
        const int _id;
        const std::string _name;
        std::queue<T *> _wait_queue;
    };
    // Initialize channel id
    template <class T>
    int Channel<T>::ID = 0;
}
#endif //_CHANNEL_HPP_
