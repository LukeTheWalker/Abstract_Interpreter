#ifndef INTERVAL_H
#define INTERVAL_H

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

template <typename T>
class Interval {
private:
    T lower;
    T upper;

    // Validate that the interval is well-formed (lower <= upper)
    void validate() const {
        if (lower > upper) {
            throw std::invalid_argument("Invalid interval: lower bound must be less than or equal to upper bound");
        }
    }

public:
    // Constructors
    Interval() : lower(std::numeric_limits<T>::lowest()), 
                 upper(std::numeric_limits<T>::max()) {}

    Interval(T l, T u) : lower(l), upper(u) {
        validate();
    }

    // Lattice Operations
    
    // Join operation (least upper bound)
    Interval join(const Interval& other) const {
        return Interval(
            std::min(this->lower, other.lower),
            std::max(this->upper, other.upper)
        );
    }

    // Meet operation (greatest lower bound)
    Interval meet(const Interval& other) const {
        return Interval(
            std::max(this->lower, other.lower),
            std::min(this->upper, other.upper)
        );
    }

    // Arithmetic Operations (E♯)
    Interval operator-() const {
        return Interval(-this->upper, -this->lower);
    }

    Interval operator+(const Interval& other) const {
        return Interval(this->lower + other.lower, this->upper + other.upper);
    }

    Interval operator-(const Interval& other) const {
        return Interval(this->lower - other.upper, this->upper - other.lower);
    }

    Interval operator*(const Interval& other) const {
        T a = this->lower * other.lower;
        T b = this->lower * other.upper;
        T c = this->upper * other.lower;
        T d = this->upper * other.upper;
        return Interval(
            std::min({a, b, c, d}),
            std::max({a, b, c, d})
        );
    }

    Interval operator/(const Interval& other) const {
        if (other.contains(0)) {
            throw std::invalid_argument("Division by zero");
        }
        T a = this->lower / other.lower;
        T b = this->lower / other.upper;
        T c = this->upper / other.lower;
        T d = this->upper / other.upper;
        return Interval(
            std::min({a, b, c, d}),
            std::max({a, b, c, d})
        );
    }

    // Comparison Operations (C♯)
    bool operator<(const Interval& other) const {
        return this->lower < other.lower && this->upper < other.upper;
    }

    bool operator==(const Interval& other) const {
        return this->lower == other.lower && this->upper == other.upper;
    }

    bool operator!=(const Interval& other) const {
        return !(*this == other);
    }

    bool operator<=(const Interval& other) const {
        return this->lower <= other.lower && this->upper <= other.upper;
    }

    bool operator>(const Interval& other) const {
        return this->lower > other.lower && this->upper > other.upper;
    }

    bool operator>=(const Interval& other) const {
        return this->lower >= other.lower && this->upper >= other.upper;
    }

    // Getters
    T getLower() const { return lower; }
    T getUpper() const { return upper; }

    // Check if interval is empty
    bool isEmpty() const {
        return lower > upper;
    }

    // Check if a value is contained in the interval
    bool contains(T value) const {
        return value >= lower && value <= upper;
    }

    // Static method to create an empty interval
    static Interval empty() {
        return Interval(
            std::numeric_limits<T>::max(), 
            std::numeric_limits<T>::lowest()
        );
    }

    // Widening operator (for abstract interpretation)
    Interval widen(const Interval& other) const {
        T new_lower = (other.lower < this->lower) ? 
            std::numeric_limits<T>::lowest() : this->lower;
        T new_upper = (other.upper > this->upper) ? 
            std::numeric_limits<T>::max() : this->upper;
        return Interval(new_lower, new_upper);
    }
};

#endif // INTERVAL_H