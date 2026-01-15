#include "mmedia/rtmp/amf/amf_number.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/mmedia_logger.h"

using namespace lssvc::mmedia;

AMFNumber::AMFNumber(const std::string &name) : AMFAny(name) {}

AMFNumber::AMFNumber() {}

AMFNumber::~AMFNumber() {}

int AMFNumber::decode(const char *data, int size, bool has) {
  if (size >= 8) {
    number_ = BytesReader::readUint64T(data);
    return 8;
  }
  return -1;
}

bool AMFNumber::isNumber() { return true; }

double AMFNumber::number() { return number_; }

void AMFNumber::dump() const { RTMP_TRACE << "Number: " << number_; }
