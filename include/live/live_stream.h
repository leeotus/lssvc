#ifndef __LIVE_STREAM_H__
#define __LIVE_STREAM_H__

#include "base/codec_utils.h"
#include "base/time_corrector.h"
#include "codec_header.h"
#include "player_user.h"
#include "gop_mgr.h"
#include "live_user.h"
#include "mmedia/base/packet.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace lssvc::live {

class LiveSession;

class LiveStream {

  static constexpr int64_t LIVE_TIMEOUT_SEC = 20 * 1000;

public:
  LiveStream(LiveSession &s, const std::string &session_name);

  // @brief get ready_time
  int64_t getReadyTime() const;

  // @brief return the duration from start time to now
  int64_t sinceStart() const;

  // @brief check whether current stream is out of time
  bool isTimeout();

  // @brief getter, data_coming_time
  int64_t getDataTime() const;

  // @brief get the session name
  const std::string &getSessionName() const;

  int32_t getStreamVersion() const;

  // @brief check whether there is video/audio/meta data
  bool hasMedia() const;

  bool ready() const;

  void addPacket(mmedia::PacketPtr &&pkt);

  void getFrames(const PlayerUserPtr &user);

private:
  void setReady(bool ready);

  bool locateGop(const PlayerUserPtr &user);
  void skipFrame(const PlayerUserPtr &user);
  void getNextFrame(const PlayerUserPtr &user);

  int64_t data_coming_time_{0};
  int64_t start_timestamp_{0};
  int64_t ready_time_{0};

  // stream pushing thread / timeout thread
  std::atomic<int64_t> stream_time_{0};

  LiveSession &session_;
  std::string session_name_;
  std::atomic<int64_t> frame_index_{-1};

  uint32_t packet_buffer_size_{1000};
  std::vector<mmedia::PacketPtr> packet_buffer_;

  bool has_audio_{false};
  bool has_video_{false};
  bool has_meta_{false};

  bool ready_{false};

  std::atomic<int32_t> stream_version_{-1};

  GopMgr gop_mgr_;
  CodecHeader codec_headers_;
  TimeCorrector time_corrector_;
  std::mutex lock_;
};

} // namespace lssvc::live

#endif
