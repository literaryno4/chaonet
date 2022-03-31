//
// Created by chao on 2022/3/31.
//

#ifndef CHAONET_TIMESTAMP_H
#define CHAONET_TIMESTAMP_H
#include <sys/types.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std::chrono;

namespace chaonet {

class Timestamp {
   public:
    typedef std::chrono::system_clock Clock;
    typedef std::chrono::time_point<Clock> TimePoint;

    Timestamp() : timePoint_(TimePoint()) {}

    explicit Timestamp(TimePoint time) : timePoint_(time) {}

    void swap(Timestamp &that) { std::swap(timePoint_, that.timePoint_); }

    std::string toString() const { return toFormattedString(); }

    std::string toFormattedString(bool showMicroseconds = true) const {
        auto t = Clock::to_time_t(timePoint_);
        std::ostringstream os;
        os << std::put_time(std::localtime(&t), "%F %T.");
        if (showMicroseconds) {
            int64_t micro =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    timePoint_.time_since_epoch())
                    .count() %
                kMicroSecondsPerSecond;
            os << micro;
        }
        return os.str();
    }

    TimePoint timePoint() const { return timePoint_; }

    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                timePoint_.time_since_epoch())
                .count());
    }

    static Timestamp now() { return Timestamp(Clock::now()); }

    static Timestamp invalid() { return Timestamp(); }
    static const int kMicroSecondsPerSecond = 1000 * 1000;

   private:
    TimePoint timePoint_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.timePoint() < rhs.timePoint();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.timePoint() == rhs.timePoint();
}

inline double timeDifference(Timestamp high, Timestamp low) {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               (high.timePoint() - low.timePoint()))
               .count() /
           Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double s) {
    return Timestamp(
        (timestamp.timePoint() + std::chrono::microseconds(static_cast<int64_t>(
                                     s * Timestamp::kMicroSecondsPerSecond))));
}

}  // namespace chaonet

#endif  // CHAONET_TIMESTAMP_H
