#include "utils/lssvc_filelog.h"
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace lssvc::utils;

bool LSSFileLog::open(const std::string &filepath) {
  file_path_ = filepath;
  int fd =
  // DEFFILEMODE: all users can read and write
      ::open(file_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
  if (fd < 0) {
    std::cout << "open file log error. Path: " << filepath << "\r\n";
    return false;
  }
  fd_ = fd;
  return true;
}

size_t LSSFileLog::writeLog(const std::string &msg) {
  int fd = fd_ == -1 ? 1 : fd_; // stdout if fd == -1
  return ::write(fd, msg.data(), msg.size());
}

void lssvc::utils::LSSFileLog::rotate(const std::string &file) {
  if (file_path_.empty()) {
    return;
  }
  int ret = ::rename(file_path_.c_str(), file.c_str());
  if (ret != 0) {
    std::cerr << "rename failed. old: " << file_path_ << ", new: " << file
              << "\r\n";
    return;
  }
  int fd =
      ::open(file_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
  if(fd < 0) {
    std::cout << "open file log error. Path: " << file <<"\r\n";
    return;
  }
  ::dup2(fd, fd_);
  close(fd);
}

void LSSFileLog::setRotate(RotateType type) { rotate_type_ = type; }

RotateType LSSFileLog::getRotateType() const { return rotate_type_; }

int64_t LSSFileLog::fileSize() const {
  // get file size using lseek
  return ::lseek64(fd_, 0, SEEK_END);
}

std::string LSSFileLog::filePath() const { return file_path_; }
