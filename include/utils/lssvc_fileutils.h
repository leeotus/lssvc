#ifndef __LSSVC_FILE_UTILS_H__
#define __LSSVC_FILE_UTILS_H__

#include <string>
#include "lssvc_string.h"

namespace lssvc::utils {

class LSSFileUtils {
public:
  LSSFileUtils() = delete;
  ~LSSFileUtils() = delete;

  static std::string getFilePath(const std::string &path);

  static std::string getFileName(const std::string &path);

  static std::string getFileNameWithExt(const std::string &path);

  static std::string getFileExtension(const std::string &path);
};

}   // lssvc::utils

#endif
