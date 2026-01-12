#ifndef __BYTES_WRITER_H__
#define __BYTES_WRITER_H__

#include <stdint.h>

namespace lssvc::mmedia {

class BytesWriter {
public:
  BytesWriter() = default;
  ~BytesWriter() = default;

  static int writeUint32T(char *buf, uint32_t val);
  static int writeUint24T(char *buf, uint32_t val);
  static int writeUint16T(char *buf, uint16_t val);
  static int writeUint8T(char *buf, uint8_t val);
};

} // namespace lssvc::mmedia

#endif
