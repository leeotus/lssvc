#include "live/codec_header.h"
#include "live/base/live_logger.h"
#include "live/base/time_corrector.h"
#include "mmedia/rtmp/amf/amf_object.h"
#include "utils/lssvc_time.h"

#include <fstream>

using namespace lssvc::utils;
using namespace lssvc::mmedia;
using namespace lssvc::live;

CodecHeader::CodecHeader() {
  start_timestamp_ = LSSTime::nowMs();
}

CodecHeader::~CodecHeader() {}

PacketPtr CodecHeader::getMeta(int index) {
  if (index <= 0) {
    return meta_;
  }

  // search from the end
  auto it = meta_packets_.rbegin();
  for (; it != meta_packets_.rend(); ++it) {
    PacketPtr ptr = *it;
    if (ptr->getIndex() <= index) {
      // find the first packet in which the index is less than the input one
      return ptr;
    }
  }
  return meta_; // no matter it is null or not
}

PacketPtr CodecHeader::getVideoHeader(int index) {
  if (index <= 0) {
    return video_header_;
  }
  auto it = video_header_packets_.rbegin();
  for (; it != video_header_packets_.rend(); ++it) {
    PacketPtr ptr = *it;
    if (ptr->getIndex() <= index) {
      // find the first packet in which the index is less than the input one
      return ptr;
    }
  }
  return video_header_; // no matter it is null or not
}

PacketPtr CodecHeader::getAudioHeader(int index) {
  if (index <= 0) {
    return audio_header_;
  }
  auto it = audio_header_packets_.rbegin();
  for (; it != audio_header_packets_.rend(); ++it) {
    PacketPtr ptr = *it;
    if (ptr->getIndex() <= index) {
      // find the first packet in which the index is less than the input one
      return ptr;
    }
  }
  return audio_header_; // no matter it is null or not
}

void CodecHeader::saveMeta(const PacketPtr &pkt) {
  meta_ = pkt;
  ++meta_version_;

  meta_packets_.emplace_back(pkt);
  LIVE_TRACE << "save meta, meta version:" << meta_version_
             << " size:" << pkt->getPacketSize()
             << " elapse:" << utils::LSSTime::nowMs() - start_timestamp_ << "ms";
}

void CodecHeader::parseMeta(const PacketPtr &pkt) {
  AMFObject obj;
  if (obj.decode(pkt->data(), pkt->getPacketSize())) {
    return;
  }

  std::stringstream ss;
  ss << "parse meta:";

  // video
  AMFAnyPtr width = obj.property("width");
  if (width) {
    ss << " width:" << (uint32_t)width->number();
  }
  AMFAnyPtr height = obj.property("height");
  if (height) {
    ss << " height:" << (uint32_t)height->number();
  }
  AMFAnyPtr video_codec = obj.property("videocodecid");
  if (video_codec) {
    ss << " videocodecid:" << (uint32_t)video_codec->number();
  }
  AMFAnyPtr fr = obj.property("framerate");
  if (fr) {
    ss << " framerate:" << (uint32_t)fr->number();
  }
  AMFAnyPtr video_datarate = obj.property("videodatarate");
  if (video_datarate) {
    ss << " videodatarate:" << (uint32_t)video_datarate->number();
  }

  // audio
  AMFAnyPtr audio_sample_rate = obj.property("audiosamplerate");
  if (audio_sample_rate) {
    ss << " audiosamplerate:" << (uint32_t)audio_sample_rate->number();
  }
  AMFAnyPtr audio_sample_size = obj.property("audiosamplesize");
  if (audio_sample_size) {
    ss << " audio_sample_size:" << (uint32_t)audio_sample_size->number();
  }
  AMFAnyPtr audio_codec = obj.property("audiocodecid");
  if (audio_codec) {
    ss << " audiocodecid:" << (uint32_t)audio_codec->number();
  }
  AMFAnyPtr audio_rate = obj.property("audiodatarate");
  if (audio_rate) {
    ss << " audiodatarate:" << (uint32_t)audio_rate->number();
  }
  AMFAnyPtr duration = obj.property("duration");
  if (duration) {
    ss << " duration:" << (uint32_t)duration->number();
  }
  AMFAnyPtr encode = obj.property("encoder");
  if (encode) {
    ss << " encoder:" << encode->str();
  }
  AMFAnyPtr server = obj.property("server");
  if (server) {
    ss << " server:" << server->str();
  }

  LIVE_TRACE << ss.str();
}

void CodecHeader::saveAudioHeader(const PacketPtr &pkt) {
  audio_header_ = pkt;
  ++audio_version_;

  audio_header_packets_.emplace_back(pkt);

  LIVE_TRACE << "save audio header, version:" << audio_version_
             << " size:" << pkt->getPacketSize()
             << " elapse:" << utils::LSSTime::nowMs() - start_timestamp_ << "ms";
}

void CodecHeader::saveVideoHeader(const PacketPtr &pkt) {
  video_header_ = pkt;
  ++video_version_;

  video_header_packets_.emplace_back(pkt);

  LIVE_TRACE << "save video header, version:" << video_version_
             << " size:" << pkt->getPacketSize()
             << " elapse:" << utils::LSSTime::nowMs() - start_timestamp_ << "ms";
}

bool CodecHeader::parseCodecHeader(const PacketPtr &pkt) {
  if (pkt->isMeta()) {
    saveMeta(pkt);
    parseMeta(pkt);
  } else {
    if (pkt->isAudio()) {
      saveAudioHeader(pkt);
    } else if (pkt->isVideo()) {
      saveVideoHeader(pkt);
    }
  }
  return true;
}
