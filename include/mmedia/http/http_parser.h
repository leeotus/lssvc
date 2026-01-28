#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

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

  // @brief add header's key-value pairs to the headers
  void addHeader(const std::string &key, const std::string &value);
  void addHeader(std::string &&key, std::string &&value);

private:
  HttpParserState state_{kExpectHeaders};
  int32_t current_chunk_length_{0};
  int32_t current_content_length_{0};
  bool is_stream_{false};
  bool is_chunked_{false};
  bool is_request_{false};

  // store the reason why parsing http request fails
  HttpStatusCode reason_{kUnknown};
  std::string header_;

  // key-value pairs parsed from the http header
  std::unordered_map<std::string, std::string> headers_;

  // type of http requests' method
  HttpMethod method_{kInvalid};

  HttpStatusCode code_{kUnknown}; // http status code

  // http version
  HttpVersion version_{kHttpUnknown};

  std::string path_;
  std::string query_;

  // store the main body of a http request
  PacketPtr chunk_;
};

} // namespace lssvc::mmedia

#endif
