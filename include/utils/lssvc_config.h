#ifndef __LSSVC_CONFIG_H__
#define __LSSVC_CONFIG_H__

#include "lssvc_appinfo.h"
#include "lssvc_domain_info.h"
#include "lssvc_filelog.h"
#include "lssvc_fileutils.h"
#include "lssvc_logger.h"
#include "lssvc_singleton.h"
#include "noncopyable.h"

#include <cstdint>
#include <json/json.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#define g_config_mgr lssvc::utils::LSSSingleton<lssvc::utils::LSSConfigMgr>::getInstance()

namespace lssvc::utils {

struct LogInfo {
  LogLevel level;
  std::string path;
  std::string name;
  RotateType rotate_type{kRotateNone};
};

using LogInfoPtr = std::shared_ptr<LogInfo>;

/**
 * @example json connfiguration file, "services" field:
 * "services":
 * [
 *  {
 *    "addr": "xxxxx",
 *    "port": 1935,
 *    "protocol": "rtmp",
 *    "transport": "tcp"
 *  }
 * ]
 */
struct ServiceInfo {
  std::string addr;
  uint16_t port;
  std::string protocol;
  std::string transport;
};

using ServiceInfoPtr = std::shared_ptr<ServiceInfo>;

class LSSDomainInfo;
class LSSAppInfo;

using DomainInfoPtr = std::shared_ptr<LSSDomainInfo>;

class LSSConfig {
public:
  LSSConfig() = default;
  ~LSSConfig() = default;

  /**
   * @brief load configuration file
   *
   * @param file [in] path to the configuration file
   * @return true if success, else false
   */
  bool loadConfig(const std::string &file);

  // @brief get logger settings
  LogInfoPtr &getLogInfo();

  /**
   * @brief get the service infos object
   * @return const std::vector<ServiceInfoPtr>&
   */
  const std::vector<ServiceInfoPtr> &getServiceInfos();

  const ServiceInfoPtr &getserviceInfo(const std::string &protocol,
                                       const std::string &transport);

  bool parseServiceInfo(const Json::Value &serviceObj);

  AppInfoPtr getAppInfo(const std::string &domain, const std::string &app);
  DomainInfoPtr getDomainInfo(const std::string &domain);

private:
  /**
   * @brief parse "directory" field from input json
   */
  bool parseDirectory(const Json::Value &root);

  /**
   * @brief parse domain data
   * @param path [in] directory to the domain description files
   * @return true if success, else false
   */
  bool parseDomainPath(const std::string &path);

  /**
   * @brief parse domain data from input file
   * @param file [in] domain description file
   * @return true if success, else false
   */
  bool parseDomainFile(const std::string &file);

public:
  std::string name_{};
  int32_t cpu_start_{0};
  int32_t thread_nums_{1};
  int32_t cpus_{1};

  bool parseLogInfo(const Json::Value &root);
  LogInfoPtr log_info_{};
  std::vector<ServiceInfoPtr> services_;

  std::mutex lock_;
  std::unordered_map<std::string, DomainInfoPtr> domaininfos_;
};

class LSSConfig;
using LSSConfigPtr = std::shared_ptr<LSSConfig>;

class LSSConfigMgr : public NonCopyable {
public:
  LSSConfigMgr() = default;
  ~LSSConfigMgr() = default;

  bool loadConfig(const std::string &file);

  LogInfoPtr &getLogInfo();

  LSSConfigPtr getConfig();

private:
  LSSConfigPtr config_;
  std::mutex lock_;
};

} // namespace lssvc::utils

#endif
