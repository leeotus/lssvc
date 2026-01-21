#include "live/base/codec_utils.h"

using namespace lssvc::mmedia;
using namespace lssvc::live;

bool CodecUtils::isCodecHeader(const mmedia::PacketPtr &pkt) {
  if (pkt->getPacketSize() > 1) {
    // flv header
    const char *b = pkt->data() + 1;
    if (*b == 0) {
      return true;
    }
  }
  return false;
}

bool CodecUtils::isKeyFrame(const mmedia::PacketPtr &pkt) {
  if (pkt->getPacketSize() > 0) {
    // flv header
    const char *b = pkt->data();
    return ((*b >> 4) & 0x0f) == 1; // keyframe
  }
  return false;
}
