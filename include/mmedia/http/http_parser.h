#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

#include "http_request.h"
#include "http_types.h"
#include "mmedia/base/packet.h"
#include "network/base/lssvc_msgbuffer.h"
#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace lssvc::mmedia {

enum HttpParserState {
  kExpectHeaders,
  kExpectNormalBody,
  kExpectStreamBody,
  kExpectHttpComplete,
  kExpectChunkLen,
  kExpectChunkBody,
  kExpectChunkComplete,
  kExpectLastEmptyChunk,
  kExpectContinue,
  kExpectError,
};

using HttpRequestPtr = std::shared_ptr<HttpRequest>;

class HttpParser {
public:
  HttpParser() = default;
  ~HttpParser() = default;

  /**
   * @brief parse the incoming http request
   * @param buf [in] the buffer storing the http message
   * @return HttpParserState
   */
  HttpParserState parse(network::LSSMsgBuffer &buf);

  const mmedia::PacketPtr &getChunk() const;

  HttpStatusCode getReason() const;

  // @brief clear status for the next http message
  void clearForNextHttp();
  void clearForNextChunk();

  HttpRequestPtr getHttpRequest() const {
    return req_;
  }

private:
  // @brief parse http header
  void parseHeaders();

  void parseNormalBody(network::LSSMsgBuffer &buf);

  void parseStream(network::LSSMsgBuffer &buf);

  void parseChunk(network::LSSMsgBuffer &buf);

  /**
   * @brief parse the first line of http request
   * @param line [in] store the string of first line
   * @example request: GET /index.html HTTP/1.1
   * @example response: HTTP/1.1 200 OK
   */
  void processMethodline(const std::string &line);

  HttpParserState state_{kExpectHeaders};
  int32_t current_chunk_length_{0};
  int32_t current_content_length_{0};
  bool is_stream_{false};
  bool is_chunked_{false};
  bool is_request_{false};

  // store the reason why parsing http request fails
  HttpStatusCode reason_{kUnknown};

  std::string header_;

  // store the main body of a http request
  mmedia::PacketPtr chunk_;

  HttpRequestPtr req_;
};

} // namespace lssvc::mmedia

#endif
