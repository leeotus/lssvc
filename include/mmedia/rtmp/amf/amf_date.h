#ifndef __AMF_DATE_H__
#define __AMF_DATE_H__

#include "amf_any.h"

namespace lssvc::mmedia {

class AMFDate : public AMFAny {
public:
  AMFDate(const std::string &name);
  AMFDate();
  ~AMFDate();

  int decode(const char *data, int size, bool has = false) override;

  bool isDate() override;
  double date() override;
  void dump() const override;
  int16_t utcOffset() const;

private:
  double utc_{0.0f};
  int16_t utc_offset_{0};
};

} // namespace lssvc::mmedia

#endif
