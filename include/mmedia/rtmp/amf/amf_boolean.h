#ifndef __AMF_BOOLEAN_H__
#define __AMF_BOOLEAN_H__

#include "amf_any.h"

namespace lssvc::mmedia {

class AMFBoolean : public AMFAny {
public:
  AMFBoolean(const std::string &name);
  AMFBoolean();
  ~AMFBoolean();

  int decode(const char *data, int size, bool has = false) override;
  bool isBoolean() override;
  bool boolean() override;
  void dump() const override;

private:
  bool b_{false};
};

} // namespace lssvc::mmedia

#endif
