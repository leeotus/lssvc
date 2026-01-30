#include "mmedia/http/http_context.h"
#include "mmedia/base/mmedia_logger.h"
#include <cstring>

using namespace lssvc::network;
using namespace lssvc::mmedia;

namespace {
  static std::string CHUNK_EOF = "0\r\n\r\n";
}

HttpContext::HttpContext(network::LSSEventLoop *loop,
                         const TcpConnectionPtr &conn, HttpHandler *handler)
    : loop_(loop), connection_(conn), handler_(handler) {}

int32_t HttpContext::parse(LSSMsgBuffer &buf) {
  while (buf.readableBytes() > 1) {
    auto state = http_parser_.parse(buf);
    if(state == kExpectHttpComplete || state == kExpectChunkComplete) {
      // already complete
      if (handler_) {
        handler_->onRequest(connection_, http_parser_.getHttpRequest(), http_parser_.getChunk());
      }
    } else if(state == kExpectError) {
      // error occurs
      HTTP_DEBUG << "state == kExpectError";
      // TODO: send 404 or other error codes
      connection_->forceClose();
    }
  }
  return 1;
}

bool HttpContext::postRequest(const std::string &header_and_body) {
  if(post_state_ != kHttpContextPostInit) {
    return false;
  }
  header_ = header_and_body;
  post_state_ = kHttpContextPostHttp; // update state
  connection_->send(header_.c_str(), header_.size());
  return true;
}

bool HttpContext::postRequest(const std::string &header, PacketPtr &pkt) {
  if(post_state_ != kHttpContextPostInit) {
    return false;
  }
  header_ = header;
  out_packet_ = pkt;
  post_state_ = kHttpContextPostHttp; // update state
  // send http request header first:
  connection_->send(header_.c_str(), header_.size());
  return true;
}

bool HttpContext::postRequest(HttpRequestPtr &req) {
  if(req->isChunked()) {
    postChunkHeader(req->makeHeaders());
  } else if(req->isStream()) {
    postStreamHeader(req->makeHeaders());
  } else {
    postRequest(req->appendToBuffer());
  }
  return true;
}

bool HttpContext::postChunkHeader(const std::string &header) {
  if(post_state_ != kHttpContextPostInit) {
    return false;
  }
  header_ = header;
  post_state_ = kHttpContextPostChunkHeader;
  connection_->send(header.c_str(), header.size());
  header_sent_ = true;
  return true;
}

void HttpContext::postChunk(PacketPtr &chunk) {
  out_packet_ = chunk;
  if(!header_sent_) {
    post_state_ = kHttpContextPostChunkHeader;
    connection_->send(header_.c_str(), header_.size());
    header_sent_ = true;
  } else {
    post_state_ = kHttpContextPostChunkLen;
    char buf[32] = {0,};
    sprintf(buf, "%X\r\n", out_packet_->getPacketSize());
    header_ = std::string(buf);
    connection_->send(header_.c_str(), header_.size());
  }
}

void HttpContext::postEofChunk() {
  post_state_ = kHttpContextPostChunkEOF;
  connection_->send(CHUNK_EOF.c_str(), CHUNK_EOF.size());
}

bool HttpContext::postStreamHeader(const std::string &header) {
  if(post_state_ != kHttpContextPostInit) {
    return false;
  }
  header_ = header;
  post_state_ = kHttpContextPostInit;
  connection_->send(header_.c_str(), header_.size());
  header_sent_ = true;
  return true;
}

void HttpContext::postStreamChunk(PacketPtr &pkt) {
  out_packet_ = pkt;
  if(header_sent_) {
    post_state_ = kHttpContextPostHttpStreamHeader;
    connection_->send(header_.c_str(), header_.size());
    header_sent_ = true;
  } else {
    post_state_ = kHttpContextPostHttpStreamChunk;
    connection_->send(out_packet_->data(), out_packet_->getPacketSize());
  }
}

void HttpContext::writeComplete(const TcpConnectionPtr &conn) {
  switch (post_state_)
  {
  case kHttpContextPostInit: {
    break;
  }

  case kHttpContextPostHttp: {
    // already send http body message, turn back to init stats
    post_state_ = kHttpContextPostInit;
    break;
  }

  case kHttpContextPostHttpHeader: {
    post_state_ = kHttpContextPostHttpBody;
    // send http body data
    connection_->send(out_packet_->data(), out_packet_->getPacketSize());
    break;
  }

  case kHttpContextPostHttpBody: {
    post_state_ = kHttpContextPostInit;
    break;
  }

  case kHttpContextPostChunkHeader: {
    post_state_ = kHttpContextPostChunkLen;
    char buf[32] = {0, };
    sprintf(buf, "%X\r\n", out_packet_->getPacketSize());
    header_ = std::string(buf);
    connection_->send(header_.c_str(), header_.size());
    break;
  }

  case kHttpContextPostChunkLen: {
    post_state_ = kHttpContextPostChunkBody;
    connection_->send(out_packet_->data(), out_packet_->getPacketSize());
    break;
  }

  case kHttpContextPostChunkBody: {
    post_state_ = kHttpContextPostInit;
    break;
  }

  case kHttpContextPostChunkEOF: {
    post_state_ = kHttpContextPostInit;
    break;
  }

  case kHttpContextPostHttpStreamHeader: {
    post_state_ = kHttpContextPostInit;
    break;
  }

  case kHttpContextPostHttpStreamChunk: {
    post_state_ = kHttpContextPostInit;
    break;
  }

  default:
    break;
  }
}
