#include "mmedia/base/bytes_writer.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>

using namespace lssvc::mmedia;

int BytesWriter::writeUint32T(char *buf, uint32_t val) {
  val = htonl(val);
  memcpy(buf, &val, sizeof(int32_t));
  return sizeof(int32_t);
}

int BytesWriter::writeUint24T(char *buf, uint32_t val) {
  val = htonl(val);
  char *ptr = (char *)&val;
  ptr++;
  memcpy(buf, ptr, 3);
  return 3;
}

int BytesWriter::writeUint16T(char *buf, uint16_t val) {
  val = htons(val);
  memcpy(buf, &val, sizeof(int16_t));
  return sizeof(int16_t);
}

int BytesWriter::writeUint8T(char *buf, uint8_t val) {
  char *data = (char *)&val;
  buf[0] = data[0];
  return 1;
}
