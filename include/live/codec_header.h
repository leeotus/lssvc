#ifndef __CODEC_HEADER_H__
#define __CODEC_HEADER_H__

#include "mmedia/base/packet.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace lssvc::live {

class CodecHeader {
public:
  CodecHeader();
  ~CodecHeader();

  mmedia::PacketPtr getMeta(int index);

  mmedia::PacketPtr getVideoHeader(int index);

  mmedia::PacketPtr getAudioHeader(int index);

  // @brief save meta data  into the meta_packets
  void saveMeta(const mmedia::PacketPtr &pkt);

  void parseMeta(const mmedia::PacketPtr &pkt);

  // @brief save video header (default flv header tag) into the audio_header_packets
  void saveAudioHeader(const mmedia::PacketPtr &pkt);

  // @brief save video header (default flv header tag) into the video_header_packets
  void saveVideoHeader(const mmedia::PacketPtr &pkt);

  bool parseCodecHeader(const mmedia::PacketPtr &pkt);

private:
  mmedia::PacketPtr video_header_;
  mmedia::PacketPtr audio_header_;
  mmedia::PacketPtr meta_;

  int meta_version_{0};
  int audio_version_{0};
  int video_version_{0};

  // the sequence header of video encoding
  std::vector<mmedia::PacketPtr> video_header_packets_;

  // the sequence header of audio encoding
  std::vector<mmedia::PacketPtr> audio_header_packets_;

  std::vector<mmedia::PacketPtr> meta_packets_;

  int64_t start_timestamp_{0};
};

} // lssvc::live

#endif
