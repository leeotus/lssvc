#ifndef __HTTP_UTILS_H__
#define __HTTP_UTILS_H__

#include "http_types.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace lssvc::mmedia {

class HttpUtils {
public:
  static HttpMethod parseMethod(const std::string &method);
  static HttpMethod parseMethod(std::string &&method);
  static HttpStatusCode parseStatusCode(int32_t code);
  static std::string parseStatusMessage(int32_t code);
  static ContentType parseContentType(const std::string &contentType);
  static const std::string &contentTypeToString(ContentType contentType);
  static const std::string &statusCodeToString(int code);
  static ContentType getContentType(const std::string &fileName);
  static std::string charToHex(char c);
  static bool needUrlDecoding(const std::string &url);
  static std::string urlDecode(const std::string &url);
  static std::string urlEncode(const std::string &src);

  static std::string &ltrim(std::string &str) {
    auto p = std::find_if(str.begin(), str.end(),
                          std::not1(std::ptr_fun<int, int>(std::isspace)));
    str.erase(str.begin(), p);
    return str;
  }

  static std::string &rtrim(std::string &str) {
    auto p = std::find_if(str.rbegin(), str.rend(),
                          std::not1(std::ptr_fun<int, int>(std::isspace)));
    str.erase(p.base(), str.end());
    return str;
  }

  static std::string &trim(std::string &str) {
    ltrim(rtrim(str));
    return str;
  }
};

} // namespace lssvc::mmedia

#endif
