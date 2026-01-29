#include "mmedia/http/http_request.h"
#include "utils/lssvc_string.h"
#include <algorithm>
#include <sstream>

using namespace lssvc::utils;
using namespace lssvc::mmedia;

namespace {
  static std::string string_empty;
}

HttpRequest::HttpRequest(bool is_request) : is_request_(is_request) {}

void HttpRequest::addHeader(const std::string &field, const std::string &value) {
  std::string k = field;
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);
  headers_[k] = value;
}

void HttpRequest::addHeader(std::string &&field, std::string &&value) {
  std::transform(field.begin(), field.end(), field.begin(), ::tolower);
  headers_[std::move(field)] = std::move(value);
}

void HttpRequest::removeHeader(const std::string &key) {
  std::string k = key;
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);
  headers_.erase(k);
}

const std::string &HttpRequest::getHeader(const std::string &key) const {
  std::string k = key;
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);
  auto it = headers_.find(k);
  if(it != headers_.end()) {
    return it->second;
  }
  return string_empty;
}

std::string HttpRequest::makeHeaders() {
  std::stringstream ss;
  if(is_request_) {
    appendRequestFirstLine(ss);
  } else {
    appendResponseFirstLine(ss);
  }

  for(auto const &h : headers_) {
    ss << h.first << ": " << h.second << "\r\n";
  }
  if(!body_.empty()) {
    ss << "content-length: " << body_.size() << "\r\n";
  } else {
    ss << "content-length: 0\r\n";
  }
  ss << "\r\n";
  return ss.str();
}

void HttpRequest::setQuery(const std::string &query) { query_ = query; }

void HttpRequest::setQuery(std::string &&query) { query_ = std::move(query); }

const std::string &HttpRequest::getQuery() const { return query_; }

void HttpRequest::setParameter(const std::string &key,
                               const std::string &value) {
  // NOTE: don't convert key and value to lowercase
  parameters_[key] = value;
}

void HttpRequest::setParameter(std::string &&key, std::string &&value) {
  // NOTE: don't convert key and value to lowercase
  parameters_[std::move(key)] = std::move(value);
}

const std::string &HttpRequest::getParameter(const std::string &key) const {
  auto it = parameters_.find(key);
  if(it != parameters_.end()) {
    return it->second;
  }
  return string_empty;
}

void HttpRequest::parseParameters() {
  auto list = LSSString::split(query_, "&");
  for(auto const &it : list) {
    auto pos = it.find('=');
    if(pos != std::string::npos) {
      std::string k = it.substr(0, pos);
      std::string v = it.substr(pos + 1);
      k = HttpUtils::trim(k);
      v = HttpUtils::trim(v);
      setParameter(std::move(k), std::move(v));
    }
  }
}

void HttpRequest::setMethod(const std::string &method) {
  method_ = HttpUtils::parseMethod(method);
}

void HttpRequest::setMethod(std::string &&method) {
  method_  = HttpUtils::parseMethod(std::move(method));
}

void HttpRequest::setMethod(HttpMethod method) {
  method_ = method;
}

HttpMethod HttpRequest::getMethod() const {
  return method_;
}

void HttpRequest::setVersion(HttpVersion v) {
  version_ = v;
}

void HttpRequest::setVersion(const std::string &v) {
  version_ = HttpVersion::kHttpUnknown;
  if (v.size() == 8) {
    // http/1.0
    if (v.compare(0, 6, "HTTP/1.")) {
      if (v[7] == '1') {
        version_ = HttpVersion::kHttp11;
      } else if (v[7] == '0') {
        version_ = HttpVersion::kHttp10;
      }
    }
  }
}

HttpVersion HttpRequest::getVersion() const { return version_; }

void HttpRequest::setPath(const std::string &path) {
  if (HttpUtils::needUrlDecoding(path)) {
    path_ = HttpUtils::urlDecode(path);
  } else {
    path_ = path;
  }
}

const std::string &HttpRequest::getPath() const { return path_; }

void HttpRequest::setStatusCode(int32_t code) { code_ = code; }

uint32_t HttpRequest::getStatusCode() const { return code_; }

void HttpRequest::setBody(const std::string &body) { body_ = body; }

void HttpRequest::setBody(std::string &&body) { body_ = std::move(body); }

const std::string &HttpRequest::getBody() const { return body_; }

void HttpRequest::appendRequestFirstLine(std::stringstream &ss) {
  switch(method_) {
    case kGet: {
      ss << "GET ";
      break;
    }

    case kPost: {
      ss << "POST ";
      break;
    }

    case kHead: {
      ss << "HEAD ";
      break;
    }

    case kPut: {
      ss << "PUT ";
      break;
    }

    case kDelete: {
      ss << "DELETE ";
      break;
    }

    case kOptions: {
      ss << "OPTIONS ";
      break;
    }

    case kPatch: {
      ss << "PATCH ";
      break;
    }

    default: {
      ss << "UNKNOW ";
      break;
    }
  }

  std::stringstream sss;
  if(!path_.empty()) {
    sss << path_;
  } else {
    sss << "/";
  }

  if(!parameters_.empty()) {
    sss << "?";
    for(auto it = parameters_.begin(); it != parameters_.end(); ++it) {
      if(it == parameters_.begin()) {
        sss << it->first << "=" << it->second;
      } else {
        sss << "&" << it->first << "=" << it->second;
      }
    }
  }
  ss << HttpUtils::urlEncode(sss.str()) << " ";
  if(version_ == HttpVersion::kHttp10) {
    ss << "HTTP/1.0 ";
  } else {
    ss << "HTTP/1.1 ";
  }
  ss << "\r\n";
}

void HttpRequest::appendResponseFirstLine(std::stringstream &ss) {
  if(version_ == HttpVersion::kHttp10) {
    ss << "HTTP/1.0 ";
  } else {
    ss << "HTTP/1.1 ";
  }

  ss << code_ << " ";
  ss << HttpUtils::parseStatusMessage(code_);
  ss << "\r\n";
}

std::string HttpRequest::appendToBuffer() {
  std::stringstream ss;
  ss << makeHeaders();
  if(!body_.empty()) {
    ss << body_;
  }
  return ss.str();
}

bool HttpRequest::isRequest() const { return is_request_; }

bool HttpRequest::isStream() const { return is_stream_; }

bool HttpRequest::isChunked() const { return is_chunked_; }

void HttpRequest::setIsStream(bool s) { is_stream_ = s; }

void HttpRequest::setIsChunked(bool c) { is_chunked_ = c; }
