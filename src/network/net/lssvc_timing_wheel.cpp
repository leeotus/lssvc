#include "network/net/lssvc_timing_wheel.h"
#include "network/base/lssvc_netlogger.h"

using namespace lssvc::network;

LSSTimingWheel::LSSTimingWheel() {
  wheels_[kTimingTypeSecond].resize(60);
  wheels_[kTimingTypeMinute].resize(60);
  wheels_[kTimingTypeHour].resize(24);
  wheels_[kTimingTypeDay].resize(30);
}

LSSTimingWheel::~LSSTimingWheel() {}

void LSSTimingWheel::insertSecondEntry(uint32_t delay, EntryPtr entry) {
  wheels_[kTimingTypeSecond][delay - 1].emplace(entry);
}

void LSSTimingWheel::insertMinuteEntry(uint32_t delay, EntryPtr entry) {
  auto minute = delay / kTimingMinute;
  auto second = delay % kTimingMinute;
  CallbackEntryPtr ep =
      std::make_shared<TimerCallbackEntry>([this, second, entry]() {
        // do migration to the 'second' wheel when 'minute' part runs out
        insertEntry(second, entry);
      });
  wheels_[kTimingTypeMinute][minute - 1].emplace(ep);
}

void LSSTimingWheel::insertHourEntry(uint32_t delay, EntryPtr entry) {
  auto hour = delay / kTimingHour;
  auto minute = delay % kTimingHour;
  CallbackEntryPtr ep =
      std::make_shared<TimerCallbackEntry>([this, minute, entry]() {
        // do migration to the 'minute' wheel when 'hour' part runs out
        insertEntry(minute, entry);
      });
  wheels_[kTimingTypeHour][hour - 1].emplace(ep);
}

void LSSTimingWheel::insertDayEntry(uint32_t delay, EntryPtr entry) {
  auto day = delay / kTimingDay;
  auto hour = delay % kTimingDay;
  CallbackEntryPtr ep =
      std::make_shared<TimerCallbackEntry>([this, hour, entry] {
        // do migration to the 'hour' wheel when 'day' part runs out
        insertEntry(hour, entry);
      });
  wheels_[kTimingTypeDay][day - 1].emplace(ep);
}

void LSSTimingWheel::insertEntry(uint32_t delay, EntryPtr entry) {
  if (delay <= 0) {
    // invalid delay param.
    entry.reset();
  }

  if (delay < kTimingMinute) {
    insertSecondEntry(delay, entry);
  }

  else if (delay < kTimingHour) {
    insertMinuteEntry(delay, entry);
  }

  else if (delay < kTimingDay) {
    insertHourEntry(delay, entry);
  }

  else {
    auto day = delay / kTimingDay;
    if (day > 30) {
      NETWORK_WARN << "day: " << day << " > 30 days is not supported yet\r\n";
      return;
    }
    insertDayEntry(delay, entry);
  }
}

void LSSTimingWheel::onTimer(int64_t now) {
  if (last_ts_ == 0) {
    last_ts_ = now;
  }
  if (now - last_ts_ < 1000) { // ms
    return;
  }
  last_ts_ = now;
  ++tick_;
  popUp(wheels_[kTimingTypeSecond]);
  if (tick_ % kTimingMinute == 0) {
    popUp(wheels_[kTimingTypeMinute]);
  } else if (tick_ % kTimingHour == 0) {
    popUp(wheels_[kTimingTypeHour]);
  } else if (tick_ % kTimingDay == 0) {
    popUp(wheels_[kTimingTypeDay]);
  }
}

void LSSTimingWheel::popUp(Wheel &bq) {
  WheelEntry tmp;

  // swap with an empty WheelEntry to trigger destruction, in this way,
  // the tasks holded in the current wheel can be executed
  bq.front().swap(tmp);
  bq.pop_front();
  bq.push_back(WheelEntry());
}

void LSSTimingWheel::runAfter(int delay, const std::function<void()> &cb) {
  CallbackEntryPtr entry =
      std::make_shared<TimerCallbackEntry>([cb]() { cb(); });
  insertEntry(delay, entry);
}

void LSSTimingWheel::runAfter(int delay, std::function<void()> &&cb) {
  CallbackEntryPtr entry =
      std::make_shared<TimerCallbackEntry>([cb]() { cb(); });
  insertEntry(delay, entry);
}

void LSSTimingWheel::runEvery(int interval, const std::function<void()> &cb) {
  CallbackEntryPtr entry =
      std::make_shared<TimerCallbackEntry>([this, cb, interval]() {
        cb();
        runEvery(interval, cb);
      });
  insertEntry(interval, entry);
}

void LSSTimingWheel::runEvery(int interval, std::function<void()> &&cb) {
  CallbackEntryPtr entry =
      std::make_shared<TimerCallbackEntry>([this, cb, interval]() {
        cb();
        runEvery(interval, cb);
      });
  insertEntry(interval, entry);
}
