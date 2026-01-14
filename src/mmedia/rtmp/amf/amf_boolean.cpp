#include "mmedia/rtmp/amf/amf_boolean.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/base/bytes_reader.h"

using namespace lssvc::mmedia;

AMFBoolean::AMFBoolean(const std::string &name) : AMFAny(name) {}

AMFBoolean::AMFBoolean() {}

AMFBoolean::~AMFBoolean() {}

int AMFBoolean::decode(const char *data, int size, bool has) {
  if(size >= 1) {
    b_ = *data != 0 ? true : false;
    return 1;
  }
  return -1;
}

bool AMFBoolean::isBoolean() {
  return true;
}

bool AMFBoolean::boolean() {
  return b_;
}

void AMFBoolean::dump() const {
  RTMP_TRACE << "Boolean: " << b_;
}
