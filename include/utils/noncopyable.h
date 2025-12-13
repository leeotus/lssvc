#ifndef __NONCOPYABLE_H__
#define __NONCOPYABLE_H__

namespace lssvc::utils {

// A kind of flags indicating that the child of this class should not be copyable
class NonCopyable {
protected:
  NonCopyable() {}
  ~NonCopyable() {}

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace lssve::utils

#endif
