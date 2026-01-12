#ifndef __BYTES_READER_H__
#define __BYTES_READER_H__

#include <stdint.h>

namespace lssvc::mmedia {

class BytesReader {
public:
  BytesReader() = default;
  ~BytesReader() = default;

  static uint32_t readUint32T(const char *data);
  static uint32_t readUint24T(const char *data);
  static uint16_t readUint16T(const char *data);
  static uint8_t readUint8T(const char *data);
};

}   // namespace lssvc::mmedia

#endif
