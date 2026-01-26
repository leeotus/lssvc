#ifndef __PLAY_USER_H__
#define __PLAY_USER_H__

#include "live_user.h"
#include "mmedia/base/packet.h"
#include "base/time_corrector.h"
#include <vector>

namespace lssvc::live {

class PlayerUser : public LiveUser {
public:
  friend class LiveStream;
  explicit PlayerUser(const network::ConnectionPtr &ptr,
                      const StreamPtr &stream, const SessionPtr &s);

  // getter
  mmedia::PacketPtr getMeta() const;
  mmedia::PacketPtr getVideoHeader() const;
  mmedia::PacketPtr getAudioHeader() const;
  TimeCorrector &getTimeCorrector();

  // clear data
  void clearMeta();
  void clearVideoHeader();
  void clearAudioHeader();

  //@note for different protocols' implementation
  virtual bool postFrames() = 0;

protected:
  mmedia::PacketPtr video_header_; // video header packet
  mmedia::PacketPtr audio_header_; // audio header packet
  mmedia::PacketPtr meta_;         // meta packet

  // status:
  bool wait_meta_{true};
  bool wait_audio_{true};
  bool wait_video_{true};

  // stores the corresponding packets' index
  int32_t video_header_index_{0};
  int32_t audio_header_index_{0};
  int32_t meta_index_{0};

  TimeCorrector time_corrector_;
  bool wait_timeout_{false};
  int32_t out_version_{-1};
  int32_t out_frame_timestamp_{0};

  std::vector<mmedia::PacketPtr> out_frames_;
  int32_t out_index_{-1};
};

using PlayerUserPtr = std::shared_ptr<PlayerUser>;

} // namespace lssvc::live

#endif
