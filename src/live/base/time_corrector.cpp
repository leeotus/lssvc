#include "live/base/time_corrector.h"
#include "live/base/codec_utils.h"
#include "live/base/live_logger.h"

using namespace lssvc::live;

uint32_t TimeCorrector::correctTimestamp(const mmedia::PacketPtr &pkt) {
  if (!CodecUtils::isCodecHeader(pkt)) {
    // LIVE_TRACE << "ts:" << pkt->getTimestamp() << " size: " << pkt->getPacketSize();
    int32_t type = pkt->getPacketType();
    if (type == mmedia::kPacketTypeVideo) {
      // video packet
      return correctVideoTimestampByVideo(pkt);
    } else if (type == mmedia::kPacketTypeAudio) {
      // audio packet
      return correctAudioTimestampByVideo(pkt);
    }
  }
  return 0;
}

uint32_t
TimeCorrector::correctAudioTimestampByVideo(const mmedia::PacketPtr &pkt) {
  ++audio_numbers_between_video_;
  if(audio_numbers_between_video_ > 1) {
    // receive continuous audio frames
    return correctAudioTimestampByAudio(pkt);
  }

  int64_t time = pkt->getTimestamp();
  if(video_original_timestamp_ == -1) {
    // haven't receive video frame
    audio_original_timestamp_ = time;
    audio_corrected_timestamp_ = time;
    return time;
  }

  int64_t delta = time - video_original_timestamp_;
  bool fine = (delta > -kMaxVideoDeltaTime) && (delta < kMaxVideoDeltaTime);
  if(!fine) {
    delta = kDefaultVideoDeltaTime;
  }

  audio_original_timestamp_ = time;
  audio_corrected_timestamp_ = video_corrected_timestamp_ + delta;
  if(audio_corrected_timestamp_ < 0) {
    audio_corrected_timestamp_ = 0;
  }
  return audio_corrected_timestamp_;
}

uint32_t
TimeCorrector::correctVideoTimestampByVideo(const mmedia::PacketPtr &pkt) {
  audio_numbers_between_video_ = 0;
  int64_t time = pkt->getTimestamp();
  if (video_original_timestamp_ == -1) {
    video_original_timestamp_ = time;
    video_corrected_timestamp_ = time;

    if (audio_original_timestamp_ != -1) {
      // calculate the delta between audio's and video's timestamp
      int32_t delta = audio_original_timestamp_ - video_original_timestamp_;
      if (delta <= -kMaxVideoDeltaTime || delta >= kMaxVideoDeltaTime) {
        video_original_timestamp_ = audio_original_timestamp_;
        video_corrected_timestamp_ = audio_corrected_timestamp_;
      }
    }
  }

  int64_t delta = time - video_original_timestamp_;
  bool fine = (delta > -kMaxVideoDeltaTime) && (delta < kMaxVideoDeltaTime);
  if (!fine) {
    delta = kDefaultVideoDeltaTime;
  }

  video_original_timestamp_ = time;
  video_corrected_timestamp_ += delta;
  if (video_corrected_timestamp_ < 0) {
    video_corrected_timestamp_ = 0;
  }
  return video_corrected_timestamp_;
}

uint32_t TimeCorrector::correctAudioTimestampByAudio(const mmedia::PacketPtr &pkt) {
  int64_t time = pkt->getTimestamp();
  if(audio_original_timestamp_ == -1) {
    // first audio frame
    audio_original_timestamp_ = time;
    audio_corrected_timestamp_ = time;
    return time;
  }
  int64_t delta = time - audio_original_timestamp_;

  // normal range
  bool fine = (delta > -kMaxAudioDeltaTime) && (delta < kMaxAudioDeltaTime);
  if(!fine) {
    // to be corrected
    delta = kDefaultAudioDeltaTime;
  }

  audio_original_timestamp_ = time;
  audio_corrected_timestamp_ += delta;
  if(audio_corrected_timestamp_ < 0) {
    audio_corrected_timestamp_ = 0;
  }
  return audio_corrected_timestamp_;
}
