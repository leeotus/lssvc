#include "utils/lssvc_config.h"
#include "utils/lssvc_logstream.h"
#include <fstream>

using namespace lssvc::utils;

bool LSSConfigMgr::LSSConfig::loadConfig(
    const std::string &file) {
  LSSVC_LOG_DEBUG << "config file:" << file << "\r\n";
  Json::Value root;
  Json::CharReaderBuilder reader;
  std::ifstream in(file); // open file
  std::string err;        // error message
  bool ret = Json::parseFromStream(reader, in, &root, &err);
  if (!ret) {
    // failed to parse configuration file
    LSSVC_LOG_ERROR << "config file:" << file << " failed to parse(" << err
                    << ").\r\n";
    return false;
  }

  Json::Value nameObj = root["name"];
  if (!nameObj.isNull()) {
    name_ = nameObj.asString();
  }
  Json::Value cpusObj = root["cpu_start"];
  if (!cpusObj.isNull()) {
    cpu_start_ = cpusObj.asInt();
  }
  Json::Value threadsObj = root["threads"];
  if (!threadsObj.isNull()) {
    thread_nums_ = threadsObj.asInt();
  }

  Json::Value logObj = root["log"];
  if (!logObj.isNull()) {
    parseLogInfo(logObj);
  }

  return true;
}

LogInfoPtr &LSSConfigMgr::LSSConfig::getLogInfo() { return log_info_; }

bool LSSConfigMgr::LSSConfig::parseLogInfo(const Json::Value &root) {
  log_info_ = std::make_shared<LogInfo>();

  // @todo lowercase before the following process
  Json::Value levelObj = root["level"]; // log level
  if (!levelObj.isNull()) {
    std::string level = levelObj.asString();
    if (level == "TRACE") {
      log_info_->level = kTrace;
    } else if (level == "DEBUG") {
      log_info_->level = kDebug;
    } else if (level == "INFO") {
      log_info_->level = kInfo;
    } else if (level == "WARN") {
      log_info_->level = kWarn;
    } else if (level == "ERROR") {
      log_info_->level = kError;
    }
  }
  Json::Value pathObj = root["path"]; // path to the log file
  if (!pathObj.isNull()) {
    log_info_->path = pathObj.asString();
  }
  Json::Value nameObj = root["name"]; // name of the log file
  if (!nameObj.isNull()) {
    log_info_->name = nameObj.asString();
  }
  Json::Value rtObj = root["rotate"];
  if (!rtObj.isNull()) {
    std::string rt = rtObj.asString();
    if (rt == "DAY") {
      log_info_->rotate_type = kRotateDay;
    } else if (rt == "HOUR") {
      log_info_->rotate_type = kRotateHour;
    }
  }
  return true;
}

LSSConfigMgr::LSSConfigPtr LSSConfigMgr::getConfig() { return config_; }

bool LSSConfigMgr::loadConfig(const std::string &file) {
  LSSConfigMgr::LSSConfigPtr config = std::make_shared<LSSConfig>();
  if(config->loadConfig(file)) {
    // pass to the LSSConfig
    std::lock_guard<std::mutex> lock(lock_);
    config_ = config;
    return true;
  }
  return false;
}

LogInfoPtr &LSSConfigMgr::getLogInfo() {
  return config_->getLogInfo();
}
