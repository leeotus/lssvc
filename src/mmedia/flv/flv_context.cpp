#include "mmedia/flv/flv_context.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/rtmp_header.h"
#include <sstream>
#include <string>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;

/**------------------------------------------------------------------------
 *!                           FLV Headers
 *------------------------------------------------------------------------**/

static char flv_audio_only_header[] = {
    0x46,                         /* 'F' */
    0x4c,                         /* 'L' */
    0x56,                         /* 'V' */
    0x01,                         /* version = 1 */
    0x04, 0x00, 0x00, 0x00, 0x09, /* header size */
};

static char flv_video_only_header[] = {
    0x46,                         /* 'F' */
    0x4c,                         /* 'L' */
    0x56,                         /* 'V' */
    0x01,                         /* version = 1 */
    0x01, 0x00, 0x00, 0x00, 0x09, /* header size */
};

static char flv_header[] = {
    0x46,                   /* 'F' */
    0x4c,                   /* 'L' */
    0x56,                   /* 'V' */
    0x01,                   /* version = 1 */
    0x05,                   /* 00000 1 0 1 = has audio & video */
    0x00, 0x00, 0x00, 0x09, /* header size */
};

/**------------------------------------------------------------------------
 *!                           FlvContext
 *------------------------------------------------------------------------**/

FlvContext::FlvContext(const network::TcpConnectionPtr &conn,
                       MMediaHandler *handler)
    : connection_(conn), handler_(handler) {
  current_ = out_buffer_;
}

void FlvContext::sendFlvHttpHeader(bool has_video, bool has_audio) {
  std::stringstream ss;
  ss << "HTTP/1.1 200 OK \r\n";
  ss << "Access-Control-Allow-Origin: *\r\n";
  ss << "Content-Type: video/x-flv\r\n";
  // set http connection "Keep-Alive"
  ss << "Connection: Keep-Alive\r\n";
  ss << "\r\n";

  http_header_ = std::move(ss.str());
  auto header_node = std::make_shared<BufferNode>((void *)http_header_.data(),
                                                  http_header_.size());
  bufs_.emplace_back(std::move(header_node));
  writeFlvHeader(has_video, has_audio);
  send();
}

void FlvContext::writeFlvHeader(bool has_video, bool has_audio) {
  char *header = current_;
  if (!has_audio) {
    // only video
    memcpy(current_, flv_video_only_header, sizeof(flv_video_only_header));
    current_ += sizeof(flv_video_only_header);
  } else if (!has_video) {
    // only audio
    memcpy(current_, flv_audio_only_header, sizeof(flv_audio_only_header));
    current_ += sizeof(flv_audio_only_header);
  } else {
    // both video and audio
    memcpy(current_, flv_header, sizeof(flv_header));
    current_ += sizeof(flv_header);
  }
  auto h = std::make_shared<BufferNode>(header, current_ - header);
  bufs_.emplace_back(std::move(h));
}

void FlvContext::send() {
  if(sending_) {
    // previous sending task not done yet
    return;
  }
  sending_ = true;
  connection_->send(bufs_);
}

char FlvContext::getRtmpPacketType(PacketPtr &pkt) {
  if (pkt->isAudio()) {
    return kRtmpMsgTypeAudio;
  } else if (pkt->isVideo()) {
    return kRtmpMsgTypeVideo;
  } else if (pkt->isMeta()) {
    return kRtmpMsgTypeAMFMeta;
  }
  return 0;
}

bool FlvContext::buildFlvFrame(PacketPtr &pkt, uint32_t timestamp) {
  out_packets_.emplace_back(pkt);
  char *header = current_;

  // previous tag size
  char *p = (char *)&previous_size_;
  *current_++ = p[3];
  *current_++ = p[2];
  *current_++ = p[1];
  *current_++ = p[0];

  // tag type
  *current_++ = getRtmpPacketType(pkt);

  // data size
  auto mlen = pkt->getPacketSize();
  p = (char *)&mlen;
  *current_++ = p[2];
  *current_++ = p[1];
  *current_++ = p[0];

  // timestamp
  p = (char *)&timestamp;
  *current_++ = p[2];
  *current_++ = p[1];
  *current_++ = p[0];
  *current_++ = 0;

  // stream id, default 0
  *current_++ = 0;
  *current_++ = 0;
  *current_++ = 0;

  // update previous_size
  previous_size_ = mlen + 11;

  auto h = std::make_shared<BufferNode>(header, current_ - header);
  bufs_.emplace_back(std::move(h));

  auto c = std::make_shared<BufferNode>(pkt->data(), pkt->getPacketSize());
  bufs_.emplace_back(std::move(c));
  return true;
}

void FlvContext::writeComplete(const network::TcpConnectionPtr &conn) {
  sending_ = false;
  current_ = out_buffer_;
  out_packets_.clear();
  if (handler_) {
    handler_->onActive(conn);
  }
}

bool FlvContext::ready() const { return !sending_; }
