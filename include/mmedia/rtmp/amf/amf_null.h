#ifndef __AMF_NULL_H__
#define __AMF_NULL_H__

#include "amf_any.h"

namespace lssvc::mmedia {

class AMFNull : public AMFAny {
public:
  AMFNull(const std::string &name);
  AMFNull();
  ~AMFNull();

  int decode(const char *data, int size, bool has = false) override;
  bool isNull() override;
  void dump() const override;

private:
  bool b_{false};
};

} // namespace lssvc::mmedia

#endif
