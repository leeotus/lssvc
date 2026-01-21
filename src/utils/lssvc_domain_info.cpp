#include "utils/lssvc_domain_info.h"
#include "utils/lssvc_appinfo.h"
#include "utils/lssvc_logger.h"
#include "utils/lssvc_logstream.h"

#include "json/json.h"
#include <fstream>

using namespace lssvc::utils;

const std::string &LSSDomainInfo::getDomainName() const {
  return name_;
}

const std::string &LSSDomainInfo::getType() const {
  return type_;
}

bool LSSDomainInfo::parseDomainInfo(const std::string &filepath) {
  LSSVC_LOG_DEBUG << "domain filapath:" << filepath;
  Json::Value root;
  Json::CharReaderBuilder reader;
  std::ifstream in(filepath);
  std::string err;
  auto ok = Json::parseFromStream(reader, in, &root, &err);
  if(!ok) {
    // failed
    LSSVC_LOG_ERROR << "failed to parse domain configuration:" << err;
    return false;
  }
  Json::Value domainObj = root["domain"];
  if(domainObj.isNull()) {
    LSSVC_LOG_ERROR << "invalid content: has no domain field.";
    return false;
  }

  // domain->name
  Json::Value nameObj = domainObj["name"];
  if(!nameObj.isNull()) {
    name_ = nameObj.asString();
  }

  // domain->type
  Json::Value typeObj = domainObj["type"];
  if(!typeObj.isNull()) {
    type_ = typeObj.asString();
  }

  // domain->app
  Json::Value appsObj = domainObj["app"];
  if(appsObj.isNull()) {
    LSSVC_LOG_ERROR << "invalid content: has no app field.";
    return false;
  }

  for(auto &app : appsObj) {
    AppInfoPtr info = std::make_shared<LSSAppInfo>(*this);
    auto ret = info->parseAppInfo(app);
    if(ret) {
      std::lock_guard<std::mutex> lock(lock_);
      appinfos_.emplace(info->app_name_, info);
    }
  }

  return true;
}

AppInfoPtr LSSDomainInfo::getAppInfo(const std::string &app_name) {
  std::lock_guard<std::mutex> lock(lock_);
  if (appinfos_.find(app_name) != appinfos_.end()) {
    return appinfos_[app_name];
  }
  return AppInfoPtr();
}
