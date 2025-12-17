#ifndef __LSSVC_TIMING_WHEEL_H__
#define __LSSVC_TIMING_WHEEL_H__

#include <array>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

constexpr int kTimingMinute = 60; // s
constexpr int kTimingHour = 60 * 60;
constexpr int kTimingDay = 60 * 60 * 24;

namespace lssvc::network {

enum TimingType {
  kTimingTypeSecond = 0,
  kTimingTypeMinute,
  kTimingTypeHour,
  kTimingTypeDay,
  kTimingTotalCnt, // total number of TimingTypes
};

using EntryPtr = std::shared_ptr<void>;
using WheelEntry = std::unordered_set<EntryPtr>;
using Wheel = std::deque<WheelEntry>;
using Wheels = std::array<Wheel, kTimingTotalCnt>;

/**
 * @brief use this class object to store timer function callback
 * execute the callback after the object is destructed (RAII)
 */
class TimerCallbackEntry {
public:
  TimerCallbackEntry(const std::function<void()> &cb) : cb_(cb) {}
  TimerCallbackEntry(const std::function<void()> &&cb) : cb_(std::move(cb)) {}

  ~TimerCallbackEntry() {
    if (cb_) {
      cb_(); // execute callback when destruction
    }
  }

private:
  std::function<void()> cb_;
};

using CallbackEntryPtr = std::shared_ptr<TimerCallbackEntry>;

class LSSTimingWheel {
public:
  LSSTimingWheel();
  ~LSSTimingWheel();

  void insertEntry(uint32_t delay, EntryPtr entry);

  void onTimer(int64_t now);
  void popUp(Wheel &bq);

  /**
   * @brief enqueue a task(callback) into the epoll, execute once
   * @param delay [in] after 'delay' seconds, execute callback
   * @param cb [in] callback function
   */
  void runAfter(int delay, const std::function<void()> &cb);
  void runAfter(int delay, std::function<void()> &&cb);

  /**
   * @brief enqueue a task(callback) into the epoll, execute it every
   * 'interval' seconds
   * @param interval [in] inerval - seconds
   * @param cb [in] callback function
   */
  void runEvery(int interval, const std::function<void()> &cb);
  void runEvery(int interval, std::function<void()> &&cb);

private:
  // @brief insert second level tasks into the wheels
  void insertSecondEntry(uint32_t delay, EntryPtr entry);

  // @brief insert minute level tasks into the wheels
  void insertMinuteEntry(uint32_t delay, EntryPtr entry);

  // @brief insert hour level tasks into the wheels
  void insertHourEntry(uint32_t delay, EntryPtr entry);

  // @brief insert day level tasks into the wheels
  void insertDayEntry(uint32_t delay, EntryPtr entry);

  Wheels wheels_;
  int64_t last_ts_{0};
  uint64_t tick_{0};
};

} // namespace lssvc::network

#endif
