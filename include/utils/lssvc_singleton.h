#ifndef __LSSVC_SINGLETON_H__
#define __LSSVC_SINGLETON_H__

#include "noncopyable.h"
#include <thread>
#include "assert.h"
#include <memory>

namespace lssvc::utils {

template <typename T>
class LSSSingleton : public NonCopyable {
public:
  LSSSingleton() = delete;
  ~LSSSingleton() = delete;

#if POSIX_INSTANCE
  static T*& getInstance() {
    // highly depends on POSIX lib.
    pthread_once(&ponce_, &LSSSingleton::init);   // ensure the init function be called only once.
    assert(value_ != nullptr);
    return value_;
  }
#else

  static T* getInstance() {
    static T t; // static variable and it's thread-safely(since c++11)
    return &t;
  }

#endif

#if POSIX_INSTANCE
private:
  static void init() {
    value_ = new T();
  }

  static pthread_once_t ponce_;
  static T* value_;
#endif

};

} // namespace lssvc::utils

#endif
