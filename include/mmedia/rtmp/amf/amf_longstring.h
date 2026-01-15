#ifndef __AMF_LONGSTRING_H__
#define __AMF_LONGSTRING_H__

#include "amf_any.h"

namespace lssvc::mmedia {

class AMFLongString : public AMFAny  {
public:
  AMFLongString(const std::string &name);
  AMFLongString();
  ~AMFLongString();

  int decode(const char *data, int size, bool has = false) override;
  bool isString() override;
  const std::string &str() override;
  void dump() const override;
private:
  std::string string_;
};

} // namespace lssvc::mmedia

#endif
