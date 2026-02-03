#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "http_types.h"
#include "http_utils.h"
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace lssvc::mmedia {

class HttpRequest;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;

class HttpRequest {
public:
  explicit HttpRequest(bool is_request = true);

  /**
   * @brief add key-value pair in the http header to the headers
   * @param field [in] key
   * @param value [in] value
   */
  void addHeader(const std::string &field, const std::string &value);
  void addHeader(std::string &&field, std::string &&value);

  // @brief remove one possible kv pair in the headers
  void removeHeader(const std::string &key);

  // @brief return the whole headers
  const std::unordered_map<std::string, std::string> &getHeaders() const;

  // @brief return the value of the specific input key in the headers
  const std::string &getHeader(const std::string &key) const;

  // @brief encapsulate HTTP header data
  std::string makeHeaders();

  void setQuery(const std::string &query);
  void setQuery(std::string &&query);

  const std::string &getQuery() const;

  void setParameter(const std::string &key, const std::string &value); // lvalue
  void setParameter(std::string &&key, std::string &&value);           // rvalue

  /**
   * @brief get the corresponding parameter according to the input key
   * @param key [in] key of parameter
   */
  const std::string &getParameter(const std::string &key) const;

  // @brief set http requests' method
  void setMethod(const std::string &method);
  void setMethod(std::string &&method);
  void setMethod(HttpMethod method);

  HttpMethod getMethod() const;

  // @brief set http version
  void setVersion(HttpVersion v);
  void setVersion(const std::string &v);

  HttpVersion getVersion() const;

  // @brief set path of http request
  void setPath(const std::string &path);
  const std::string &getPath() const; // getter

  // @breif set http status code
  void setStatusCode(int32_t code);
  uint32_t getStatusCode() const; // getter

  // @brief set http requests' body message
  void setBody(const std::string &body);
  void setBody(std::string &&body);
  const std::string &getBody() const; // getter

  std::string appendToBuffer();

  bool isRequest() const;
  bool isStream() const;
  bool isChunked() const;

  void setIsStream(bool s);
  void setIsChunked(bool c);

  static HttpRequestPtr newHttp400Response();
  static HttpRequestPtr newHttp404Response();
private:
  // @brief generate and append http requests' first line
  void appendRequestFirstLine(std::stringstream &ss);
  // @brief generate and append http responses' first line
  void appendResponseFirstLine(std::stringstream &ss);

  // @brief parse parameter
  void parseParameters();

  HttpMethod method_{kInvalid};
  HttpVersion version_{HttpVersion::kHttpUnknown};
  std::string path_;
  std::string query_;
  std::unordered_map<std::string, std::string> headers_;
  std::unordered_map<std::string, std::string> parameters_;
  std::string body_;
  uint32_t code_{0};
  bool is_request_{true};
  bool is_stream_{false};
  bool is_chunked_{false};
};

} // namespace lssvc::mmedia

#endif
