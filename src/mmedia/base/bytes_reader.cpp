#include "mmedia/base/bytes_reader.h"
#include <cstdint>
#include <netinet/in.h>
#include <cstring>

using namespace lssvc::mmedia;

uint64_t BytesReader::readUint64T(const char *data) {
  uint64_t in = *((uint64_t *)data);
  uint64_t res = __bswap_64(in);
  double value;
  memcpy(&value, &res, sizeof(double));
  return value;

}

uint32_t BytesReader::readUint32T(const char *data) {
  uint32_t *c = (uint32_t *)data;
  return ntohl(*c);
}

uint32_t BytesReader::readUint24T(const char *data) {
  unsigned char *c = (unsigned char *)data;
  uint32_t val;
  val = (c[0] << 16) | (c[1] << 8) | c[2];
  return val;
}

uint16_t BytesReader::readUint16T(const char *data) {
  uint16_t *c = (uint16_t *)data;
  return ntohs(*c);
}

uint8_t BytesReader::readUint8T(const char *data) { return data[0]; }
