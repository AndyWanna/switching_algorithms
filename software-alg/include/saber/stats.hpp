// stats.h header file
#ifndef  STATS_H
#define  STATS_H

#include <iostream>
#include <utility>
#include <set>
#include <limits>
#include <map>
#include <cassert>

#ifdef ENABLE_HDRHISTOGRAM
#include <hdr/hdr_histogram.h>
#endif

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace saber {
//! Basic statistic class
    class Stats {
    protected:
        size_t _num_samples{0};
        double _sample_sum{0.0};
        double _sample_squared_sum{0.0};
        double _min{std::numeric_limits<double>::max()};
        double _max{std::numeric_limits<double>::min()};

        std::pair<size_t/* num of samples */, double /* average */> _average_cache{0, 0.0};
        std::pair<size_t/* num of samples */, double /* variable */> _variance_cache{0, 0.0};

        bool _enable_histogram{false};
        const int _offset{1};
#ifdef ENABLE_HDRHISTOGRAM
        struct hdr_histogram *_hist{nullptr};
#endif
        const std::string _name;

        /* reversed for extension */
        std::map<std::string, double> _simple_counter;
#ifdef ENABLE_HDRHISTOGRAM
        std::map<std::string, hdr_histogram *> _more_histogram;
#endif

        bool no_sample_yet() const {
            return _num_samples == 0;
        }

        //! Hidden constructor
        explicit Stats(std::string name, bool enable_histogram, int offset) :
                _enable_histogram(enable_histogram), _offset(offset),
                _name(std::move(name)) {
            assert (!_name.empty() && "Argument name MUST NOT be empty");
            assert(offset > 0 && "Argument offset MUST be a positive integer");
#ifdef ENABLE_HDRHISTOGRAM
            if (_enable_histogram) {
      // Initialise the histogram
      hdr_init(
          _offset,  // Minimum value
          INT64_C(3600000000),  // Maximum value
          3,  // Number of significant figures
          &_hist);  // Pointer to initialise
    }
#endif
        }

    public:
        static Stats *New(const json &conf) {
            Stats *result = nullptr;
            assert(conf.count("name") && "Argument conf MUST contain the parameter for name");
            std::string name = conf["name"].get<std::string>();
            bool enable_histogram = false;
            int offset = 1;
#ifdef ENABLE_HDRHISTOGRAM
            if (conf.count("enable_histogram")) {
      enable_histogram = conf["enable_histogram"].get<bool>();
    }
#endif
            if (conf.count("offset")) {
                offset = conf["offset"].get<int>();
            }
//            if (name == "general") {
            result = new Stats(name, enable_histogram, offset);
//            } else {
//                std::cerr << "Unknown name : " << name << "\n";
//            }

            return result;
        }

        virtual ~Stats() {
#ifdef ENABLE_HDRHISTOGRAM
            if (_enable_histogram) {
      if (_hist)
        delete(_hist);

      for (auto &hist : _more_histogram) {
        if (hist.second) {
          delete(hist.second);
        }
      }
      }
#endif
        }

        double average() {
            if (no_sample_yet()) {
                // std::cerr << "WARNING: no sample yet!\n";
                return std::numeric_limits<double>::quiet_NaN();
            }
            if (_average_cache.first != _num_samples) {
                _average_cache.second = _sample_sum / _num_samples;
            }
            return _average_cache.second;
        }

        double min() const {
            if (no_sample_yet()) {
               // std::cerr << "WARNING: no sample yet!\n";
            }
            return _min;
        }

        double max() const {
            if (no_sample_yet()) {
               // std::cerr << "WARNING: no sample yet!\n";
            }
            return _max;
        }

#ifdef ENABLE_HDRHISTOGRAM
        double get_hdr_min() const {
    if (_enable_histogram) {
      return hdr_min(_hist) - _offset;
    }
    return std::numeric_limits<double>::quiet_NaN();
  }

  double get_hdr_mean() const {
    if (_enable_histogram) {
      return hdr_mean(_hist) - _offset;
    }
    return std::numeric_limits<double>::quiet_NaN();
  }

  double get_hdr_max() const {
    if (_enable_histogram) {
      return hdr_max(_hist) - _offset;
    }
    return std::numeric_limits<double>::quiet_NaN();
  }

  double get_hdr_stddev() const {
    if (_enable_histogram) {
      return hdr_stddev(_hist);
    }
    return std::numeric_limits<double>::quiet_NaN();
  }
#endif

        double variance() {
            if (no_sample_yet()) {
               // std::cerr << "WARNING: no sample yet!\n";
                return std::numeric_limits<double>::quiet_NaN();
            }
            if (_variance_cache.first != _num_samples) {
                double avg = average();
                _variance_cache.second =
                        _sample_squared_sum / _num_samples + avg * avg - 2 * avg * _sample_sum / _num_samples;
            }
            return _variance_cache.second;
        }

        size_t num_samples() const {
            return _num_samples;
        }

        double square_sum() const {
            return _sample_squared_sum;
        }

        double sample_sum() const {
            return _sample_sum;
        }

#ifdef ENABLE_HDRHISTOGRAM
        double get_percentile(int percentage) const {
    if (_enable_histogram) {
      return hdr_value_at_percentile(_hist, percentage) - _offset;
    }
    std::cerr << "WARNING: hdr_histogram is not enabled!\n";
    return -1.0;
  }
#endif

        void add_sample(int val) {
#ifdef ENABLE_HDRHISTOGRAM
            if (_enable_histogram) {
      // Record value
      hdr_record_value(
          _hist, // Histogram to record to
          val + _offset);  // Value to record
    }
#endif
            add_sample((double) val);
        }

        void add_sample(double val) {
            _num_samples++;
            _sample_sum += val;
            _sample_squared_sum += (val * val);

            if (val > _max)
                _max = val;
            if (val < _min)
                _min = val;
        }

        virtual void display(std::ostream &os) const {
            os <<   "number of samples     : " << _num_samples
               << "\nsample sum            : " << _sample_sum
               << "\nsample square sum     : " << _sample_squared_sum
               << "\nminimum               : " << _min
               << "\nmaximum               : " << _max << "\n";
#ifdef ENABLE_HDRHISTOGRAM
            if (_enable_histogram) {
              // Print out the values of the histogram
              os <<   "hdr_mean              : " << hdr_mean(_hist) - _offset
                 << "\nhdr_min               : " << hdr_min(_hist) - _offset
                 << "\nhdr_max               : " << hdr_max(_hist) - _offset
                 << "\nhdr_stddev            : " << hdr_stddev(_hist)
                 << "\nP90                   : " << get_percentile(90)
                 << "\nP95                   : " << get_percentile(95)
                 << "\nP99                   : " << get_percentile(99)
                 << "\n";
            }
#endif
        }

        virtual void reset() {
            _num_samples = 0;
            _sample_sum = 0;
            _sample_squared_sum = 0.0;
            _average_cache.first = 0;
            _average_cache.second = 0.0;
            _variance_cache.first = 0;
            _variance_cache.second = 0.0;

            _min = std::numeric_limits<double>::max();
            _max = std::numeric_limits<double>::min();
#ifdef ENABLE_HDRHISTOGRAM
            if (_enable_histogram)
      hdr_reset(_hist);
    for (auto &hist : _more_histogram){
      hdr_reset(hist.second);
    }
#endif
            for (auto &cnt : _simple_counter) {
                cnt.second = 0.0;
            }
        }

        virtual bool add_simple_counter(const std::string &name) {
            assert(!name.empty() && "Argument name MUST NOT be empty!");
            if (_simple_counter.count(name)) {
                std::cerr << "Simple counter " << name << " already exists!\n";
                return false;
            } else {
                _simple_counter[name] = 0.0;
                return true;
            }
        }

#ifdef ENABLE_HDRHISTOGRAM
        virtual bool add_histogram(const std::string &name) {
    assert(!name.empty() && "Argument name MUST NOT be empty!");
    if (!_enable_histogram) {
      std::cerr << "Add histogram failed, since histogram is not enabled!\n";
      return false;
    } else if (_more_histogram.count(name)) {
      std::cerr << "Histogram " << name << " already exists!\n";
      return false;
    } else {
      _more_histogram[name] = nullptr;
      hdr_init(
          _offset,  // Minimum value
          INT64_C(3600000000),  // Maximum value
          3,  // Number of significant figures
          &_more_histogram[name]);  // Pointer to initialise
      return true;
    }
  }
#endif

        virtual bool increment_simple_counter(const std::string &name, int val) {
            assert(!name.empty() && "Argument name MUST NOT be empty!");
            if (!_simple_counter.count(name)) {
                std::cerr << "Cannot find simple counter " << name << "\n";
                return false;
            } else {
                _simple_counter[name] += val;
                return true;
            }
        }

#ifdef ENABLE_HDRHISTOGRAM
        virtual bool add_sample_histogram(const std::string &name, int val) {
    assert(!name.empty() && "Argument name MUST NOT be empty!");
    if (!_more_histogram.count(name)) {
      std::cerr << "Cannot find histogram " << name << "\n";
      return false;
    } else {
      hdr_record_value(_more_histogram[name], val + _offset);
      return true;
    }
  }
#endif

        virtual double get_counter(const std::string &name) const {
            if (!_simple_counter.count(name)) {
                std::cerr << "Cannot find simple counter " << name << "\n";
                return std::numeric_limits<double>::quiet_NaN();
            } else {
                return _simple_counter.at(name);
            }
        }

#ifdef ENABLE_HDRHISTOGRAM
        virtual double get_hdr_min(const std::string &name) const {
    if (!_more_histogram.count(name)) {
      std::cerr << "Cannot find histogram " << name << "\n";
      return false;
    } else {
      return hdr_min(_more_histogram.at(name)) - _offset;
    }
  }

  virtual double get_hdr_max(const std::string &name) const {
    if (!_more_histogram.count(name)) {
      std::cerr << "Cannot find histogram " << name << "\n";
      return false;
    } else {
      return hdr_max(_more_histogram.at(name)) - _offset;
    }
  }

  virtual double get_hdr_mean(const std::string &name) const {
    if (!_more_histogram.count(name)) {
      std::cerr << "Cannot find histogram " << name << "\n";
      return false;
    } else {
      return hdr_mean(_more_histogram.at(name)) - _offset;
    }
  }

  virtual double get_hdr_precentile(const std::string &name, int percentage) const {
    if (!_more_histogram.count(name)) {
      std::cerr << "Cannot find histogram " << name << "\n";
      return false;
    } else {
      return hdr_value_at_percentile(_more_histogram.at(name), percentage) - _offset;
    }
  }
#endif // Enable_HdrHistogram_C

        // reserved function for extensions
        virtual void add_ext(const std::string &name, const std::string &type) {

        }

        virtual void add_sample_ext(const std::string &name, const std::string &type, double val) {

        }

        virtual double get_ext(const std::string &name, const std::string &type, const std::string &sub_type) {
            return -1.0;
        }

    }; // class Stats
} // namespace saber
#endif // STATS_H
