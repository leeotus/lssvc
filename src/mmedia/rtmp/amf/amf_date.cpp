#include "mmedia/rtmp/amf/amf_date.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/mmedia_logger.h"

using namespace lssvc::mmedia;

AMFDate::AMFDate(const std::string &name) : AMFAny(name) {}

AMFDate::AMFDate() {}

AMFDate::~AMFDate() {}

int AMFDate::decode(const char *data, int size, bool has) {
  if (size < 10) {
    return -1;
  }
  utc_ = BytesReader::readUint64T(data);
  data += 8;
  utc_offset_ = BytesReader::readUint16T(data);
  return 10;
}

bool AMFDate::isDate() { return true; }

double AMFDate::date() { return utc_; }

void AMFDate::dump() const {
  RTMP_TRACE << "Date: " << utc_ << ", " << utc_offset_;
}
