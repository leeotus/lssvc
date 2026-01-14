#include "mmedia/rtmp/amf/amf_longstring.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/mmedia_logger.h"

using namespace lssvc::mmedia;

AMFLongString::AMFLongString(const std::string &name) : AMFAny(name) {}

AMFLongString::AMFLongString() {}

AMFLongString::~AMFLongString() {}

int AMFLongString::decode(const char *data, int size, bool has) {
  if (size < 2) {
    return -1;
  }
  auto len = BytesReader::readUint16T(data);
  if (len < 0 || size < len + 2) {
    return -1;
  }
  string_.assign(data + 2, len);
  return len + 2;
}

bool AMFLongString::isString() { return true; }

const std::string &AMFLongString::str() { return string_; }

void AMFLongString::dump() const { RTMP_TRACE << "LongString: " << string_; }
