#include "mmedia/rtmp/amf/amf_null.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/base/bytes_reader.h"

using namespace lssvc::mmedia;

AMFNull::AMFNull(const std::string &name) : AMFAny(name) {}

AMFNull::AMFNull() {}

AMFNull::~AMFNull() {}

int AMFNull::decode(const char *data, int size, bool has) { return 0; }

bool AMFNull::isNull() { return true; }

void AMFNull::dump() const { RTMP_TRACE << "Null "; }

