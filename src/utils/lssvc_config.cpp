#include "utils/lssvc_config.h"
#include "utils/lssvc_appinfo.h"
#include "utils/lssvc_domain_info.h"
#include "utils/lssvc_logstream.h"
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace lssvc::utils;

namespace {
  static ServiceInfoPtr service_info_nullptr;
}

bool LSSConfig::loadConfig(const std::string &file) {
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

  // parse "services" informations
  if(!parseServiceInfo(root["services"])) {
    return false;
  }

  parseDirectory(root["directory"]);
  return true;
}

LogInfoPtr &LSSConfig::getLogInfo() { return log_info_; }

bool LSSConfig::parseLogInfo(const Json::Value &root) {

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

const std::vector<ServiceInfoPtr> &LSSConfig::getServiceInfos() {
  return services_;
}

const ServiceInfoPtr &
LSSConfig::getserviceInfo(const std::string &protocol,
                                        const std::string &transport) {
  for (auto &s : services_) {
    if (s->protocol == protocol && s->transport == transport) {
      return s;
    }
  }
  return service_info_nullptr;
}

bool LSSConfig::parseServiceInfo(const Json::Value &serviceObj) {
  if (serviceObj.isNull()) {
    LSSVC_LOG_ERROR << "config no service section";
    return false;
  }
  if (!serviceObj.isArray()) {
    LSSVC_LOG_ERROR << "service section type is not an array";
    return false;
  }
  for(auto const &s : serviceObj) {
    ServiceInfoPtr sinfo = std::make_shared<ServiceInfo>();
    sinfo->addr =
        s.get("addr", "127.0.0.1").asString();   // default address "0.0.0.0"
    sinfo->port = s.get("port", "1935").asInt(); // default port "1935"
    sinfo->protocol =
        s.get("protocol", "rtmp").asString(); // default protocol "rtmp"
    sinfo->transport =
        s.get("transport", "tcp").asString(); // default use "tcp"

    LSSVC_LOG_INFO << "service address:" << sinfo->addr
                   << ", port:" << sinfo->port
                   << ", protocol:" << sinfo->protocol
                   << ", transport:" << sinfo->transport;
    services_.emplace_back(sinfo);
  }
  return true;
}

bool LSSConfig::parseDirectory(const Json::Value &root) {
  if(root.isNull() || !root.isArray()) {
    return false;
  }
  for(const Json::Value &d : root) {
    std::string path = d.asString();
    struct stat st; // to check "path" indicates to a file or a directory
    LSSVC_LOG_TRACE << "path:" << path;

    if(stat(path.c_str(), &st) != -1) {
      if((st.st_mode & S_IFMT) == S_IFDIR) {
        // directory
        parseDomainPath(path);
      } else if ((st.st_mode & S_IFREG)) {
        // file
        parseDomainFile(path);
      }
    }
  }
  return true;
}

bool LSSConfig::parseDomainPath(const std::string &path) {
  DIR *dp = nullptr;
  struct dirent *pp = nullptr;
  LSSVC_LOG_DEBUG << "parse domain path:" << path;
  dp = opendir(path.c_str());
  if (dp == nullptr) {
    // failed;
    return false;
  }
  while(true) {
    pp = readdir(dp);
    if(pp == nullptr) {
      break;
    }
    if(pp->d_name[0] == '.') {
      // '.', '..' directory
      continue;
    }
    if (pp->d_type == DT_REG) {
      // normal file
      if (path.at(path.size() - 1) != '/' || path.at(path.size() - 1) != '\\') {
        parseDomainFile(path + "/" + pp->d_name);
      } else {
        parseDomainFile(path + pp->d_name);
      }
    }
  }
  closedir(dp);
  return true;
}

bool LSSConfig::parseDomainFile(const std::string &file) {
  LSSVC_LOG_DEBUG << "parsing domain file:" << file;
  DomainInfoPtr d = std::make_shared<LSSDomainInfo>();
  bool ret = d->parseDomainInfo(file);
  if (ret) {
    std::lock_guard<std::mutex> lock(lock_);
    if (domaininfos_.find(d->getDomainName()) != domaininfos_.end()) {
      domaininfos_.erase(d->getDomainName());
    }
    domaininfos_.emplace(d->getDomainName(), d);
  }
  return true;
}

AppInfoPtr LSSConfig::getAppInfo(const std::string &domain, const std::string &app) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = domaininfos_.find(domain);
  if(it != domaininfos_.end()) {
    return it->second->getAppInfo(app);
  }
  return AppInfoPtr{};
}

DomainInfoPtr LSSConfig::getDomainInfo(const std::string &domain) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = domaininfos_.find(domain);
  if(it != domaininfos_.end()) {
    return it->second;
  }
  return DomainInfoPtr{};
}
