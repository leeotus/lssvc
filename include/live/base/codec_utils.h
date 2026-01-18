#ifndef __CODEC_UTILS_H__
#define __CODEC_UTILS_H__

#include "mmedia/base/packet.h"

namespace lssvc::live {

class CodecUtils {
public:
  static bool isCodecHeader(const mmedia::PacketPtr &pkt);
};

} // namespace lssvc::live

#endif
