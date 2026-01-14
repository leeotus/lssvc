#ifndef __AMF_STRING_H__
#define __AMF_STRING_H__

#include "amf_any.h"

namespace lssvc::mmedia {

class AMFString : public AMFAny {
public:
  AMFString(const std::string &name);
  AMFString();
  ~AMFString();

  int decode(const char *data, int size, bool has = false) override;
  bool isString() override;
  const std::string &str() override;
  void dump() const override;

private:
  std::string string_;
};

} // namespace lssvc::mmedia

#endif
