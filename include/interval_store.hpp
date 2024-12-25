#ifndef ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP
#define ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP

#include <map>
#include <string>
#include <iostream>
#include "interval.hpp"

template <typename T>
class IntervalStore
{
public:
    IntervalStore();
    void add_interval(std::string var, T start, T end);
    bool is_present(std::string var);
    void remove_interval(std::string var);
    void print_intervals();
    void clear();

private:
    std::map<std::string, Interval> intervals;
};

template <typename T>
IntervalStore<T>::IntervalStore()
{
}

template <typename T>
void IntervalStore<T>::add_interval(std::string var, T start, T end)
{
    intervals[var] = Interval(start, end);
}

template <typename T>
bool IntervalStore<T>::is_present(std::string var)
{
    return intervals.find(var) != intervals.end();
}

template <typename T>
void IntervalStore<T>::remove_interval(std::string var)
{
    intervals.erase(var);
}

template <typename T>
void IntervalStore<T>::print_intervals()
{
    for (auto const &pair : intervals)
    {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
}

template <typename T>
void IntervalStore<T>::clear()
{
    intervals.clear();
}


#endif // ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP
// Compare this snippet from include/intervals.hpp: