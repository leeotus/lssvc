#include "live/rtmp_playeruser.h"
#include "live/base/live_logger.h"
#include "live/live_stream.h"
#include "mmedia/rtmp/rtmp_context.h"
#include "utils/lssvc_time.h"

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::live;

RtmpPlayerUser::RtmpPlayerUser(const network::ConnectionPtr &conn,
                               const StreamPtr &stream, const SessionPtr &s)
    : PlayerUser(conn, stream, s) {}

bool RtmpPlayerUser::postFrames() {
  if(!stream_->ready() || !stream_->hasMedia()) {
    // not ready to play yet
    return false;
  }
  stream_->getFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
  if(meta_) {
    bool ret = pushFrame(meta_, true);
    if(ret) {
      LIVE_INFO << "rtmp send meta (ts:" << LSSTime::nowMs() << ") to host:" << user_id_;
      meta_.reset();
    }
  } else if(audio_header_) {
    bool ret = pushFrame(audio_header_, true);
    if(ret) {
      LIVE_INFO << "rtmp send audio_header (ts:" << LSSTime::nowMs() << ") to host:" << user_id_;
      audio_header_.reset();
    }
  } else if(video_header_) {
    bool ret = pushFrame(video_header_, true);
    if(ret) {
      LIVE_INFO << "rtmp send video_header (ts:" << LSSTime::nowMs() << ") to host:" << user_id_;
      video_header_.reset();
    }
  } else if(!out_frames_.empty()) {
    bool ret = pushFrames(out_frames_);
    if(ret) {
      out_frames_.clear();
    }
  } else {
    deactive();
  }
  return true;
}

UserType RtmpPlayerUser::getUserType() const {
  return UserType::kUserTypePlayerRtmp;
}

bool RtmpPlayerUser::pushFrame(PacketPtr &pkt, bool is_header) {
  auto cx = connection_->getContext<RtmpContext>(kRtmpContext);
  if(!cx || !cx->ready()) {
    return false;
  }
  int64_t ts = 0;
  if(!is_header) {
    ts = time_corrector_.correctTimestamp(pkt);
  }
  cx->buildChunk(pkt, ts, is_header);
  cx->send();
  return true;
}

bool RtmpPlayerUser::pushFrames(std::vector<PacketPtr> &list) {
  auto cx = connection_->getContext<RtmpContext>(kRtmpContext);
  if (!cx || !cx->ready()) {
    return false;
  }
  int64_t ts = 0;
  for (int i = 0; i < list.size(); ++i) {
    PacketPtr &pkt = list[i];
    ts = time_corrector_.correctTimestamp(pkt);
    cx->buildChunk(pkt, ts);
  }
  cx->send();
  return true;
}
