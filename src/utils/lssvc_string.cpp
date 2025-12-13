#include "utils/lssvc_string.h"
#include <string.h>

using namespace lssvc::utils;

bool LSSString::startWith(const std::string &s, const std::string &sub)
{
  if(s.empty()) {
    return false;
  }
  if(sub.empty()) {
    return true;
  }
  size_t len = s.size();
  size_t slen = sub.size();
  if(slen > len) {
    return false;
  }
  return s.compare(0, slen, sub) == 0;
}

bool LSSString::endsWith(const std::string &s, const std::string &sub) {
  if(sub.empty()) {
    return true;
  }
  if(s.empty()) {
    return false;
  }
  size_t len = s.size();
  size_t slen = sub.size();
  if(slen > len) {
    return false;
  }
  // return strncmp(s.c_str()+len-slen, sub.c_str(), len) == 0; // or
  return s.compare(len-slen, slen, sub) == 0;
}

std::vector<std::string> LSSString::split(const std::string &s, const std::string &delim, bool acceptEmpty) {
  if(delim.empty()) {
    return std::vector<std::string>{};
  }
  std::vector<std::string> v;
    size_t last = 0;
    size_t next = 0;
    while ((next = s.find(delim, last)) != std::string::npos) {
      if (next > last || acceptEmpty)
        v.push_back(s.substr(last, next - last));
      last = next + delim.length();
    }
    if (s.length() > last || acceptEmpty)
      v.push_back(s.substr(last));
    return v;
}
