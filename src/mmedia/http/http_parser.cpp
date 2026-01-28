#include "mmedia/http/http_parser.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/http/http_types.h"
#include "utils/lssvc_string.h"
#include <algorithm>

using namespace lssvc::network;
using namespace lssvc::mmedia;

// end of a http message
static std::string CRLFCRLF = "\r\n\r\n";
static int32_t kHttpBodySize = 64 * 1024;

namespace {
  static std::string string_empty;
}

HttpParserState HttpParser::parse(LSSMsgBuffer &buf) {

}

void HttpParser::addHeader(const std::string &key, const std::string &value) {

}

void HttpParser::addHeader(std::string &&key, std::string &&value) {

}
