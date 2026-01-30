#include "mmedia/http/http_parser.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/http/http_types.h"
#include "utils/lssvc_string.h"
#include <algorithm>
#include <ctype.h>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;

// end of a http header
static std::string CRLFCRLF = "\r\n\r\n";
static int32_t kHttpMaxBodySize = 64 * 1024;

namespace {
  static std::string string_empty;
}

HttpParserState HttpParser::parse(LSSMsgBuffer &buf) {
  if(buf.readableBytes() == 0) {
    // empty
    return state_;  // return current state
  }
  switch (state_)
  {
  case kExpectHeaders: {
    // parse http header
    if(buf.readableBytes() > CRLFCRLF.size()) {
      // haven't complete yet
      // search the end of the http header
      auto *space =
          std::search(buf.peek(), (const char *)buf.beginWrite(),
                      CRLFCRLF.data(), CRLFCRLF.data() + CRLFCRLF.size());
      if(space != (const char *)buf.beginWrite()) {
        auto size = space - buf.peek();
        header_.assign(buf.peek(), size); // store the http header
        buf.retrieve(size);
        parseHeaders();
        if(state_ == kExpectHttpComplete || state_ == kExpectError) {
          return state_;
        }
      } else {
        if (buf.readableBytes() > kHttpMaxBodySize) {
          // error
          reason_ = k400BadRequest;
          state_ = kExpectError;
          return state_;
        }
        return kExpectContinue;
      }
    } else {
      return kExpectContinue;
    }
  } break;

  case kExpectNormalBody: {
    parseNormalBody(buf);
    break;
  }

  case kExpectStreamBody: {
    parseStream(buf);
    break;
  }

  case kExpectChunkLen: {
    auto crlf = buf.findCRLF();
    if(crlf) {
      std::string len(buf.peek(), crlf);
      char *end;
      current_chunk_length_ = std::strtol(len.c_str(), &end, 16);
      HTTP_DEBUG << "chunk len:" << current_chunk_length_;

      if(current_chunk_length_ > 1024 * 1024) {
        // error
        HTTP_ERROR << "error chunk len";
        state_ = kExpectError;
        reason_ = k400BadRequest;
      }
      buf.retrieveUntil(crlf + 2);
      if(current_chunk_length_ == 0) {
        chunk_.reset();
        state_ = kExpectLastEmptyChunk;
      } else {
        state_ = kExpectChunkBody;
      }
    } else {
      if(buf.readableBytes() > 32) {
        buf.retrieveAll();
        reason_ = k400BadRequest;
        state_ = kExpectError;
        return state_;
      }
    }
    break;
  }

  case kExpectChunkBody: {
    parseChunk(buf);
    if(state_ == kExpectChunkComplete) {
      return state_;
    }
    break;
  }

  case kExpectLastEmptyChunk: {
    auto crlf = buf.findCRLF();
    if(crlf) {
      buf.retrieveUntil(crlf + 2);
      chunk_.reset();
      state_ = kExpectChunkComplete;
      break;
    }
  }

  default:
    break;
  }

  return state_;
}

void HttpParser::parseHeaders() {
  // parsing http header
  // each line is end with "\r\n"
  auto list = LSSString::split(header_, "\r\n");
  if (list.size() < 1) {
    // error
    reason_ = k400BadRequest;
    state_ = kExpectError;
    return;
  }
  processMethodline(list[0]);
  for(auto &it : list) {
    // key:value pairs
    auto pos = it.find_first_of(':');
    if(pos != std::string::npos) {
      std::string key = it.substr(0, pos);
      std::string value = it.substr(pos+1);

      HTTP_DEBUG << "parse header key:" << key << " value:" << value;
      req_->addHeader(std::move(key), std::move(value));
    }
  }

  // get the value of content-length field
  auto len = req_->getHeader("content-length");
  if(!len.empty()) {
    HTTP_TRACE << "content-length:" << len;
    try {
      current_content_length_ = std::stoull(len);
    } catch(...) {
      reason_ = k400BadRequest;
      state_ = kExpectError;
      return;
    }

    if(current_content_length_ == 0) {
      // already complete
      state_ = kExpectHttpComplete;
    } else {
      state_ = kExpectNormalBody;
    }
  } else {
    const std::string &chunk = req_->getHeader("transfer-encoding");
    if(!chunk.empty() && chunk == "chunked") {
      // http chunk transfering
      is_chunked_ = true;
      req_->setIsChunked(true);
      state_ = kExpectChunkLen;
    } else {
      if ((!is_request_ && req_->getStatusCode() != 200) ||
          (is_request_ &&
           (req_->getMethod() == kGet || req_->getMethod() == kHead ||
            req_->getMethod() == kOptions))) {
        // has no body
        current_chunk_length_ = 0;
        state_ = kExpectHttpComplete;
      } else {
        // http streaming mode
        current_content_length_ = -1;
        is_stream_ = true;
        state_ = kExpectStreamBody;
      }
    }
  }
}

void HttpParser::parseNormalBody(network::LSSMsgBuffer &buf) {
  if (!chunk_) {
    // allocate a new packet to store http body
    chunk_ = Packet::newPacket(current_content_length_);
  }
  auto size = std::min((int)buf.readableBytes(), chunk_->getSpace());
  memcpy(chunk_->data() + chunk_->getPacketSize(), buf.peek(), size);
  chunk_->updatePacketSize(size);
  buf.retrieve(size);
  current_content_length_ -= size;
  if (current_content_length_ == 0) {
    state_ = kExpectHttpComplete;
  }
}

void HttpParser::parseStream(network::LSSMsgBuffer &buf) {
  if (!chunk_) {
    // allocate a new packet to store http body
    chunk_ = Packet::newPacket(kHttpMaxBodySize);
  }
  auto size = std::min((int)buf.readableBytes(), chunk_->getSpace());
  memcpy(chunk_->data() + chunk_->getPacketSize(), buf.peek(), size);
  chunk_->updatePacketSize(size);
  buf.retrieve(size);

  if (chunk_->getSpace() == 0) {
    state_ = kExpectChunkComplete;
  }
}

void HttpParser::parseChunk(network::LSSMsgBuffer &buf) {
  if(!chunk_) {
    // allocate a new packet to store http body
    chunk_ = Packet::newPacket(current_chunk_length_);
  }
  auto size = std::min((int)buf.readableBytes(), chunk_->getSpace());
  memcpy(chunk_->data() + chunk_->getPacketSize(), buf.peek(), size);
  chunk_->updatePacketSize(size);
  buf.retrieve(size);
  current_chunk_length_ -= size;
  if(current_chunk_length_ == 0 || chunk_->getSpace() == 0) {
    state_ = kExpectChunkComplete;
  }
}

void HttpParser::processMethodline(const std::string &line) {
  HTTP_DEBUG << "parse method line:" << line;
  auto list = LSSString::split(line, " ");
  if(list.size() != 3) {
    reason_ = k400BadRequest;
    state_ = kExpectError;
    return;
  }

  std::string str = list[0];
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  if (str[0] == 'h' && str[1] == 't' && str[2] == 't' && str[3] == 'p') {
    is_request_ = false;
  }
  if(req_) {
    req_.reset();
  }
  req_ = std::make_shared<HttpRequest>(is_request_);

  if(is_request_) {
    req_->setMethod(list[0]);
    const std::string &path = list[1];
    auto pos = path.find_first_of("?");
    if(pos != std::string::npos) {
      req_->setPath(path.substr(0, pos));
      req_->setPath(path.substr(pos + 1));
    } else {
      req_->setPath(path);
    }
    req_->setVersion(list[2]);

    HTTP_DEBUG << "http method:" << list[0] << " path:" << req_->getPath()
               << " query:" << req_->getQuery() << " version:" << list[2];
  } else {
    req_->setVersion(list[0]);
    req_->setStatusCode(std::atoi(list[1].c_str()));
    HTTP_DEBUG << "http code:" << list[1] << " version:" << list[0];
  }
}

const PacketPtr &HttpParser::getChunk() const { return chunk_; }

HttpStatusCode HttpParser::getReason() const { return reason_; }

void HttpParser::clearForNextHttp() {
  state_ = kExpectHeaders;
  header_.clear();
  req_.reset();
  current_content_length_ = -1;
  chunk_.reset();
}

void HttpParser::clearForNextChunk() {
  if (is_chunked_) {
    state_ = kExpectChunkLen;
    current_chunk_length_ = -1;
  } else {
    if (is_stream_) {
      state_ = kExpectStreamBody;
    } else {
      state_ = kExpectHeaders;
      current_chunk_length_ = -1;
    }
  }
  chunk_.reset();
}
