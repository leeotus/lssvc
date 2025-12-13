#ifndef __LSSVC_STRING_H__
#define __LSSVC_STRING_H__

#include <string>
#include <vector>

namespace lssvc::utils {

class LSSString {
public:
  LSSString() = delete;
  ~LSSString() = delete;

  // @brief check whether the input string is started with 'sub'
  static bool startWith(const std::string &s, const std::string &sub);

  // @brief check whether the input string is ended with 'sub'
  static bool endsWith(const std::string &s, const std::string &sub);

  // @brief split the input string by the specific delimiter
  static std::vector<std::string> split(const std::string &s,
                                        const std::string &delim,
                                        bool acceptEmpty = false);
};

} // namespace lssvc::utils

#endif
