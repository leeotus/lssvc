#include "mmedia/rtmp/amf/amf_any.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/bytes_writer.h"
#include "mmedia/base/mmedia_logger.h"
#include <cstring>
#include <netinet/in.h>

using namespace lssvc::mmedia;

namespace {
static std::string empty_string;
}

AMFAny::AMFAny(const std::string &name) : name_(name) {}

AMFAny::AMFAny() {}

AMFAny::~AMFAny() {}

const std::string &AMFAny::str() {
  if (this->isString()) {
    return this->str();
  }
  RTMP_ERROR << "not a string.";
  return empty_string;
}

bool AMFAny::boolean() {
  if (this->isBoolean()) {
    return this->boolean();
  }
  RTMP_ERROR << "not a string.";
  return false;
}

double AMFAny::number() {
  if (this->isNumber()) {
    return this->number();
  }
  RTMP_ERROR << "not a number.";
  return 0.0f;
}

double AMFAny::date() {
  if (this->isDate()) {
    return this->date();
  }
  RTMP_ERROR << "not a date";
  return 0.0f;
}

AMFObjectPtr AMFAny::object() {
  if (this->isObject()) {
    return this->object();
  }
  RTMP_ERROR << "note an object.";
  return AMFObjectPtr();
}

bool AMFAny::isString() { return false; }

bool AMFAny::isNumber() { return false; }

bool AMFAny::isBoolean() { return false; }

bool AMFAny::isDate() { return false; }

bool AMFAny::isObject() { return false; }

bool AMFAny::isNull() { return false; }

const std::string &AMFAny::name() const { return name_; }

int32_t AMFAny::count() const { return 1; }

std::string AMFAny::decodeString(const char *data) {
  auto len = BytesReader::readUint16T(data);
  if (len > 0) {
    std::string str(data + 2, len);
    return str;
  }
  return std::string{};
}

int AMFAny::writeNumber(char *buf, double val) {
  uint64_t res;
  uint64_t in;
  memcpy(&in, &val, sizeof(double));

  res = __bswap_64(in);
  memcpy(buf, &res, 8);
  return 8;
}

int32_t AMFAny::encodeNumber(char *output, double val) {
  char *p = output;

  *p++ = kAMFNumber;
  p += writeNumber(p, val);
  return p - output;
}

int32_t AMFAny::encodeString(char *output, const std::string &str) {
  char *p = output;
  size_t len = str.size();

  *p++ = kAMFString;
  p += BytesWriter::writeUint16T(p, len);
  memcpy(p, str.c_str(), len);
  p += len;

  return p - output;
}

int32_t AMFAny::encodeBoolean(char *output, bool b) {
  char *p = output;
  *p++ = kAMFBoolean;
  *p++ = b ? 0x01 : 0x00;

  return p - output;
}

int AMFAny::encodeName(char *buf, const std::string &name) {
  // char *old = buf;
  auto len = name.size();
  unsigned short length = htons(len);
  memcpy(buf, &length, 2);
  buf += 2;

  memcpy(buf, name.c_str(), len);
  buf += len;
  return len + 2;
}

int32_t AMFAny::encodeNamedNumber(char *output, const std::string &name, double val) {
  char *old = output;
  output += encodeName(output, name);
  output += encodeNumber(output, val);
  return output - old;
}

int32_t AMFAny::encodeNamedString(char *output, const std::string &name, const std::string &val) {
  char *old = output;
  output += encodeName(output, name);
  output += encodeString(output, val);
  return output - old;
}

int32_t AMFAny::encodeNamedBoolean(char *output, const std::string &name, bool val) {
  char *old = output;
  output += encodeName(output, name);
  output += encodeBoolean(output, val);
  return output - old;
}
