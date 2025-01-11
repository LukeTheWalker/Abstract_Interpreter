// Updated interval_store.hpp
#ifndef INTERVAL_STORE_HPP
#define INTERVAL_STORE_HPP

#include <map>
#include <string>
#include "interval.hpp"

template <typename T>
class IntervalStore {
private:
    std::map<std::string, Interval<T>> intervals;

public:
    IntervalStore() = default;

    void update_interval(const std::string& var, const Interval<T>& interval) {
        intervals[var] = interval;
    }

    Interval<T> get_interval(const std::string& var) const {
        auto it = intervals.find(var);
        if (it != intervals.end()) {
            return it->second;
        }
        return Interval<T>(); // Return top interval
    }

    bool has_variable(const std::string& var) const {
        return intervals.find(var) != intervals.end();
    }

    IntervalStore join(const IntervalStore& other) const {
        IntervalStore result;
        // Join all variables from both stores
        for (const auto& [var, interval] : intervals) {
            if (other.has_variable(var)) {
                result.update_interval(var, interval.join(other.get_interval(var)));
            } else {
                result.update_interval(var, interval);
            }
        }
        // Add variables that are only in other store
        for (const auto& [var, interval] : other.intervals) {
            if (!has_variable(var)) {
                result.update_interval(var, interval);
            }
        }
        return result;
    }

    void clear() {
        intervals.clear();
    }

    void print() const {
        for (const auto& [var, interval] : intervals) {
            std::cout << var << " = [" << interval.getLower() 
                     << ", " << interval.getUpper() << "]" << std::endl;
        }
    }

    bool operator==(const IntervalStore& other) const {
        return intervals == other.intervals;
    }

    bool operator!=(const IntervalStore& other) const {
        return intervals != other.intervals;
    }
};

#endif