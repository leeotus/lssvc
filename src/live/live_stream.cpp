#include "live/live_stream.h"
#include "live/base/live_logger.h"
#include "live/live_session.h"
#include "utils/lssvc_time.h"

using namespace lssvc::utils;
using namespace lssvc::mmedia;
using namespace lssvc::live;

LiveStream::LiveStream(LiveSession &s, const std::string &session_name)
    :session_(s), session_name_(session_name), packet_buffer_(packet_buffer_size_) {
  stream_time_ = utils::LSSTime::nowMs();
  start_timestamp_ = utils::LSSTime::nowMs();
}

int64_t LiveStream::getReadyTime() const { return ready_time_; }

int64_t LiveStream::sinceStart() const {
  return LSSTime::nowMs() - start_timestamp_;
}

bool LiveStream::isTimeout() {
  auto delta = LSSTime::nowMs() - stream_time_;
  if(delta > LIVE_TIMEOUT_SEC) {
    return true;
  }
  return false;
}

int64_t LiveStream::getDataTime() const {
  return data_coming_time_;
}

const std::string &LiveStream::getSessionName() const { return session_name_; }

int32_t LiveStream::getStreamVersion() const {
  return stream_version_.load();
}

bool LiveStream::hasMedia() const {
  return has_audio_ || has_video_ || has_meta_;
}

bool LiveStream::ready() const {
  return ready_;
}

void LiveStream::setReady(bool r) {
  ready_ = true;
  ready_time_ = LSSTime::nowMs();
}

void LiveStream::addPacket(mmedia::PacketPtr &&pkt) {
  auto t = time_corrector_.correctTimestamp(pkt);
  pkt->setTimestamp(t);
  {
    std::lock_guard<std::mutex> lk(lock_);
    auto index = ++frame_index_;
    pkt->setIndex(index);
    if (pkt->isVideo() && CodecUtils::isKeyFrame(pkt)) {
      setReady(true);
      pkt->setPacketType(kPacketTypeVideo | kFrameTypeKeyFrame);
    }

    if (CodecUtils::isCodecHeader(pkt)) {
      // header
      codec_headers_.parseCodecHeader(pkt);
      if (pkt->isVideo()) {
        has_video_ = true;
        stream_version_ += 1;
      } else if (pkt->isAudio()) {
        has_audio_ = true;
        stream_version_ += 1;
      } else if (pkt->isMeta()) {
        has_meta_;
        stream_version_ += 1;
      }
    }

    gop_mgr_.addFrame(pkt);
    packet_buffer_[index % packet_buffer_size_] = std::move(pkt);
    /**
     * @note "min index" = "current frame's index" - "packet buffer size"
     * which means: the range of latest frame index should be
     * [frame_index-packet_buffer_size, frame_index_packet], therefore
     * we regard those frames of which index is less then "min index" are
     * "expired" (to be removed)
     */
    auto min_idx = frame_index_ - packet_buffer_size_;
    if (min_idx > 0) {
      gop_mgr_.clearExpiredGop(min_idx);
    }
  }

  if(data_coming_time_ == 0) {
    data_coming_time_ = LSSTime::nowMs();
  }
  stream_time_ = LSSTime::nowMs();
  auto frame = frame_index_.load();
  if(frame_index_ < 300 || frame % 5 == 0) {
    session_.activeAllPlayers();
  }
}

void LiveStream::getFrames(const PlayerUserPtr &user) {
  if(!hasMedia()) {
    return;
  }
  if (user->meta_ || user->audio_header_ || user->video_header_ ||
      !user->out_frames_.empty()) {
    // still sending old frame data
    return;
  }
  std::lock_guard<std::mutex> lk(lock_);
  if(user->out_index_ >= 0) {
    int min_index = frame_index_ - packet_buffer_size_;
    int content_latency = user->getAppInfo()->content_latency_;
    if ((user->out_index_ < min_index) ||
        (gop_mgr_.getLatestTimestamp() - user->out_frame_timestamp_) >
            2 * content_latency) {
      // need skip frames
      LIVE_INFO << "need skip, out index: " << user->out_index_
                << " min index: " << min_index
                << " out timestamp:" << user->out_frame_timestamp_
                << " latest timestamp:" << gop_mgr_.getLatestTimestamp();
      skipFrame(user);
    }
  } else {
    if (!locateGop(user)) {
      // find the correspong GOP(sequence header) for this frame
      return;
    }
  }

  // get next frame now
  getNextFrame(user);
}

bool LiveStream::locateGop(const PlayerUserPtr &user) {
  int content_latency = user->getAppInfo()->content_latency_;
  int latency{0};
  int index = gop_mgr_.getGopByLatency(content_latency, latency);
  if(index != -1) {
    user->out_index_ = index - 1;
  } else {
    auto elapse = user->elapsedTime();
    if(elapse >= 1000 && !user->wait_timeout_) {
      LIVE_DEBUG << "wait GOP keyframe timeout, host:" << user->user_id_;
      user->wait_timeout_ = true;
    }
    return false;
  }

  user->wait_meta_ = (user->wait_meta_ && has_meta_);
  if(user->wait_meta_) {
    auto meta = codec_headers_.getMeta(index);
    if(meta) {
      user->wait_meta_ = false;
      user->meta_ = meta;
      user->meta_index_ = meta->getIndex();
    }
  }

  user->wait_audio_ = (user->wait_audio_ && has_audio_);
  if(user->wait_audio_) {
    auto audio_header = codec_headers_.getAudioHeader(index);
    if(audio_header) {
      user->wait_audio_ = false;
      user->audio_header_ = audio_header;
      user->audio_header_index_ = audio_header->getIndex();
    }
  }

  user->wait_video_ = (user->wait_video_ && has_video_);
  if(user->wait_video_) {
    auto video_header = codec_headers_.getVideoHeader(index);
    if(video_header) {
      user->wait_video_ = false;
      user->video_header_ = video_header;
      user->video_header_index_ = video_header->getIndex();
    }
  }

  if(user->wait_meta_ || user->wait_audio_ || user->wait_video_ || index == -1) {
    auto elapse = user->elapsedTime();
    if(elapse >= 1000 && !user->wait_timeout_) {
      LIVE_DEBUG << "wait GOP keyframe timeout, elapse: " << elapse
                 << "ms, frame index:" << frame_index_
                 << ", gop size:" << gop_mgr_.getGopsSize()
                 << ", host:" << user->user_id_;
      user->wait_timeout_ = true;
    }
    return false;
  }

  // reset
  user->wait_meta_ = true;
  user->wait_audio_ = true;
  user->wait_video_ = true;
  user->out_version_ = stream_version_;

  auto elapsed = user->elapsedTime();
  LIVE_DEBUG << "locate GOP success, elapse: " << elapsed
             << "ms, gop index: " << index << ", frame index: " << frame_index_
             << ", latency:" << latency << ", user:" << user->user_id_;

  return true;
}

void LiveStream::skipFrame(const PlayerUserPtr &user) {
  int content_lantency = user->getAppInfo()->content_latency_;
  int latency = 0;
  auto index = gop_mgr_.getGopByLatency(content_lantency, latency);
  if(index == -1 || index <= user->out_index_) {
    return;
  }

  auto meta = codec_headers_.getMeta(index);
  if(meta) {
    if(meta->getIndex() > user->meta_index_) {
      user->meta_ = meta;
      user->meta_index_ = meta->getIndex();
    }
  }

  auto audio_header = codec_headers_.getAudioHeader(index);
  if (audio_header) {
    if (audio_header->getIndex() > user->audio_header_index_) {
      user->audio_header_ = audio_header;
      user->audio_header_index_ = audio_header->getIndex();
    }
  }

  auto video_header = codec_headers_.getVideoHeader(index);
  if (video_header) {
    if (video_header->getIndex() > user->video_header_index_) {
      user->video_header_ = video_header;
      user->video_header_index_ = video_header->getIndex();
    }
  }

  LIVE_DEBUG << "skip frame: " << user->out_index_ << "->" << index
             << ", latency:" << latency << ", frame_index:" << frame_index_
             << ", host:" << user->user_id_;

  user->out_index_ = index - 1;
}

void LiveStream::getNextFrame(const PlayerUserPtr &user) {
  auto index = user->out_index_ + 1;
  auto max_index = frame_index_.load();
  for (int i = 0; i < 10; ++i) {
    if (index > max_index) {
      break;
    }
    auto &pkt = packet_buffer_[index % packet_buffer_size_];
    if (pkt) {
      user->out_frames_.emplace_back(pkt);
      user->out_index_ = pkt->getIndex();
      user->out_frame_timestamp_ = pkt->getTimestamp();
      index = pkt->getIndex() + 1;
    } else {
      break;
    }
  }
}
