#pragma once

#include <chrono>

class Timer
{
public:
    Timer();
    ~Timer() = default;

    void restart();
    std::string str() const;

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    TimePoint start_;
};

inline std::ostream& operator<<(std::ostream& s, const Timer& timer)
{
    return s << timer.str();
}
