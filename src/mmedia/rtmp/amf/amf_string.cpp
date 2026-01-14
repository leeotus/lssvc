#include "mmedia/rtmp/amf/amf_string.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/mmedia_logger.h"

using namespace lssvc::mmedia;

AMFString::AMFString(const std::string &name) : AMFAny(name) {}

AMFString::AMFString() {}

AMFString::~AMFString() {}

int AMFString::decode(const char *data, int size, bool has) {
  if (size < 2) {
    return -1;
  }
  auto len = BytesReader::readUint16T(data);
  if (len < 0 || size < len + 2) {
    return -1;
  }
  string_ = decodeString(data);
  return len + 2;
}

bool AMFString::isString() { return true; }

const std::string &AMFString::str() { return string_; }

void AMFString::dump() const { RTMP_TRACE << "String: " << string_; }
