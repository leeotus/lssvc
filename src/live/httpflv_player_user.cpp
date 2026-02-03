#include "live/httpflv_player_user.h"
#include "live/base/live_logger.h"
#include "live/live_stream.h"
#include "mmedia/flv/flv_context.h"
#include "utils/lssvc_time.h"

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::live;

HttpFlvPlayerUser::HttpFlvPlayerUser(const network::ConnectionPtr &ptr,
                                     const StreamPtr &stream,
                                     const SessionPtr &s)
    : PlayerUser(ptr, stream, s) {}

void HttpFlvPlayerUser::pushFlvHttpHeader() {
  auto ctx = connection_->getContext<FlvContext>(kFlvContext);
  if(ctx) {
    bool has_video = stream_->hasVideo();
    bool has_audio = stream_->hasAudio();
    ctx->sendFlvHttpHeader(has_video, has_audio);

    // set http_header_sent is true
    http_header_sent_ = true;
  }
}

bool HttpFlvPlayerUser::postFrames() {
  if (!stream_->ready() || !stream_->hasMedia()) {
    deactive();
    return false;
  }
  if(!http_header_sent_) {
    // haven't send http header yet
    pushFlvHttpHeader();
    return false;
  }

  stream_->getFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
  if (meta_) {
    auto ret = pushFrame(meta_, true);
    if(ret) {
      LIVE_INFO << "rtmp sent meta now:" << LSSTime::nowMs() << " host:" << user_id_;
      meta_.reset();
    }
  } else if (audio_header_) {
    auto ret = pushFrame(audio_header_, true);
    if (ret) {
      LIVE_INFO << "rtmp sent audio_header now:" << LSSTime::nowMs()
                << " host:" << user_id_;
      audio_header_.reset();
    }
  } else if (video_header_) {
    auto ret = pushFrame(video_header_, true);
    if (ret) {
      LIVE_INFO << "rtmp sent video_header now:" << LSSTime::nowMs()
                << " host:" << user_id_;
      video_header_.reset();
    }
  } else if (!out_frames_.empty()) {
    auto ret = pushFrames(out_frames_);
    if (ret) {
      out_frames_.clear();
    }
  } else {
    deactive();
  }
  return true;
}

bool HttpFlvPlayerUser::pushFrame(mmedia::PacketPtr &pkt, bool is_header) {
  auto ctx = connection_->getContext<FlvContext>(kFlvContext);
  if (!ctx || !ctx->ready()) {
    return false;
  }
  int64_t ts = 0;
  if (!is_header) {
    ts = time_corrector_.correctTimestamp(pkt);
  }
  ctx->buildFlvFrame(pkt, ts);
  ctx->send();
  return true;
}

bool HttpFlvPlayerUser::pushFrames(std::vector<mmedia::PacketPtr> &list) {
  auto ctx = connection_->getContext<FlvContext>(kFlvContext);
  if (!ctx || !ctx->ready()) {
    return false;
  }
  int64_t ts = 0;
  for(int i = 0; i < list.size(); ++i) {
    PacketPtr &pkt = list[i];
    ts = time_corrector_.correctTimestamp(pkt);
    ctx->buildFlvFrame(pkt, ts);
  }

  ctx->send();
  return true;
}

UserType HttpFlvPlayerUser::getUserType() const {
  return UserType::kUserTypePlayerFlv;
}
