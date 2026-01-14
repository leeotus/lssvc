#ifndef __AMF_OBJECT_H__
#define __AMF_OBJECT_H__

#include "amf_any.h"
#include <vector>

namespace lssvc::mmedia {

using AMFAnyPtr = std::shared_ptr<AMFAny>;

class AMFObject: public AMFAny {
public:
  AMFObject(const std::string &name);
  AMFObject();
  ~AMFObject();

  int decode(const char *data, int size, bool has = false) override;
  bool isObject() override;
  AMFObjectPtr object() override;
  void dump() const override;

  int decodeOnce(const char *data, int size, bool has = false);
  const AMFAnyPtr &property(const std::string &name) const;
  const AMFAnyPtr &property(int index) const;

private:
  std::vector<AMFAnyPtr> properties_;
};

} // namespace lssvc::mmedia

#endif
