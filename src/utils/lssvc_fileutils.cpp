#include "utils/lssvc_fileutils.h"
#include "utils/lssvc_string.h"

using namespace lssvc::utils;

std::string LSSFileUtils::getFilePath(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos) {
    return "./";
  }
  return path.substr(0, pos);
}

std::string LSSFileUtils::getFileName(const std::string &path) {
  std::string fullname = getFileNameWithExt(path);
  size_t pos = fullname.find_last_of(".");
  if (pos != std::string::npos) {
    if (pos != 0) {
      return fullname.substr(0, pos);
    }
  }
  return fullname;
}

std::string LSSFileUtils::getFileNameWithExt(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos) {
    if (pos + 1 < path.size()) {
      return path.substr(pos + 1);
    }
  }
  return path;
}

std::string LSSFileUtils::getFileExtension(const std::string &path) {
  std::string fullname = getFileNameWithExt(path);
  size_t pos = fullname.find_last_of(".");
  if (pos != std::string::npos) {
    if (pos != 0 && pos + 1 < fullname.size()) {
      return fullname.substr(pos + 1);
    }
  }
  return std::string{};
}
