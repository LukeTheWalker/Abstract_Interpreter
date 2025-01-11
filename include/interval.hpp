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
    bool empty = false;

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
        //validate();
    }

    // Copy constructor
    Interval(const Interval& other) : lower(other.lower), upper(other.upper) {}

    // Assignment operator
    Interval& operator=(const Interval& other) {
        lower = other.lower;
        upper = other.upper;
        return *this;
    }

    // Lattice Operations
    
    // Join operation (least upper bound)
    Interval join(const Interval& other) const {
        auto it = Interval(
            std::min(this->lower, other.lower),
            std::max(this->upper, other.upper)
        );
        it.empty = this->empty && other.empty;
        return it;
    }

    // Meet operation (greatest lower bound)
    Interval meet(const Interval& other) const {
        auto it = Interval(
            std::max(this->lower, other.lower),
            std::min(this->upper, other.upper)
        );
        it.empty = this->empty || other.empty;
        return it;
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

    // Widening operator (for abstract interpretation)
    Interval widen(const Interval& other) const {
        T new_lower = (other.lower < this->lower) ? 
            std::numeric_limits<T>::lowest() : this->lower;
        T new_upper = (other.upper > this->upper) ? 
            std::numeric_limits<T>::max() : this->upper;
        return Interval(new_lower, new_upper);
    }

    static Interval build_empty() {
        auto it =  Interval(
            std::numeric_limits<T>::max(), 
            std::numeric_limits<T>::lowest()
        );
        it.empty = true;
        return it;
    }
};

#endif // INTERVAL_H