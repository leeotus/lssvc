#include "live/base/codec_utils.h"

using namespace lssvc::mmedia;
using namespace lssvc::live;

bool CodecUtils::isCodecHeader(const mmedia::PacketPtr &pkt) {
  // flv header
  if (pkt->getPacketSize() > 1) {
    const char *b = pkt->data() + 1;
    if (*b == 0) {
      return true;
    }
  }
  return false;
}
