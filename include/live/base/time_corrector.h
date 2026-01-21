#ifndef __TIME_CORRECTOR_H__
#define __TIME_CORRECTOR_H__

#include <cstdint>
#include "mmedia/base/packet.h"

namespace lssvc::live {

// TODO: add dynamic detection of timestamp drift (for example, adjusting
// through the average interval of multiple consecutive packets)
class TimeCorrector {

  static constexpr int32_t kMaxVideoDeltaTime = 100; // ms
  static constexpr int32_t kMaxAudioDeltaTime = 100;
  static constexpr int32_t kDefaultVideoDeltaTime = 40;
  static constexpr int32_t kDefaultAudioDeltaTime = 20;

public:
  TimeCorrector() = default;
  ~TimeCorrector() = default;

  uint32_t correctTimestamp(const mmedia::PacketPtr &pkt);

  /**
   * @brief correct audio timestamp
   * @param pkt [in] audio packet
   * @return uint32_t adjusted timestamp
   */
  uint32_t correctAudioTimestampByVideo(const mmedia::PacketPtr &pkt);

  /**
   * @brief correct video timestamp
   * @param pkt [in] video packet
   * @return uint32_t adjusted timestamp
   */
  uint32_t correctVideoTimestampByVideo(const mmedia::PacketPtr &pkt);

  // @note works only for continuous audio packets
  uint32_t correctAudioTimestampByAudio(const mmedia::PacketPtr &pkt);

private:
  int64_t video_original_timestamp_{-1}; // video original timestamp
  int64_t video_corrected_timestamp_{-1};

  int64_t audio_original_timestamp_{-1}; // audio original timestamp
  int64_t audio_corrected_timestamp_{-1};

  // number of audio frame between two video frames
  int32_t audio_numbers_between_video_{0};
};

} // lssvc::live

#endif
