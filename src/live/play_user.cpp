#include "live/player_user.h"
#include <string>

using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::live;

PlayerUser::PlayerUser(const network::ConnectionPtr &ptr,
                       const StreamPtr &stream, const SessionPtr &s)
    : LiveUser(ptr, stream, s) {}

PacketPtr PlayerUser::getMeta() const { return meta_; }

PacketPtr PlayerUser::getVideoHeader() const { return video_header_; }

PacketPtr PlayerUser::getAudioHeader() const { return audio_header_; }

TimeCorrector &PlayerUser::getTimeCorrector() { return time_corrector_; }

void PlayerUser::clearMeta() { meta_.reset(); }

void PlayerUser::clearVideoHeader() { video_header_.reset(); }

void PlayerUser::clearAudioHeader() { audio_header_.reset(); }
