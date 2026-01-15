#ifndef __AMF_NUMBER_H__
#define __AMF_NUMBER_H__

#include "amf_any.h"

namespace lssvc::mmedia {

class AMFNumber : public AMFAny {
public:
  AMFNumber(const std::string &name);
  AMFNumber();
  ~AMFNumber();

  int decode(const char *data, int size, bool has = false) override;
  bool isNumber() override;
  double number() override;
  void dump() const override;

private:
  double number_{0.0f};
};

} // namespace lssvc::mmedia

#endif
