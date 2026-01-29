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

  /**
   * @brief get the value in the header pairs. (key-value)
   * @param key [in] key of the pair
   */
  const std::string &getHeader(const std::string &key);

  // @brief return headers
  const std::unordered_map<std::string, std::string> &getHeaders() const;

  // @brief get the method of http message
  const std::string &getMethod() const;

  // @brief get the version of http message
  const std::string &getVersion() const;

  // @brief get the http code
  uint32_t getCode() const;

  // @brief return the request path of a http request
  const std::string &getPath() const;

  // @brief return the query string of a http request
  const std::string &getQuery() const;

  const mmedia::PacketPtr &getChunk() const;

  HttpStatusCode getReason() const;

  // @brief whether it is request or response
  bool isRequest() const;

  // @brief clear status for the next http message
  void clearForNextHttp();
  void clearForNextChunk();

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

  // key-value pairs parsed from the http header
  std::unordered_map<std::string, std::string> headers_;

  // type of http requests' method
  std::string method_;

  uint32_t code_{0}; // http status code

  // http version
  std::string version_;

  std::string path_;
  std::string query_;

  // store the main body of a http request
  mmedia::PacketPtr chunk_;
};

} // namespace lssvc::mmedia

#endif
