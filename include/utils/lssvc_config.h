#ifndef __LSSVC_CONFIG_H__
#define __LSSVC_CONFIG_H__

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

#define g_config_mgr lssvc::utils::LSSSingleton<lssvc::utils::LSSConfigMgr>::getInstance()

namespace lssvc::utils {

struct LogInfo {
  LogLevel level;
  std::string path;
  std::string name;
  RotateType rotate_type{kRotateNone};
};

using LogInfoPtr = std::shared_ptr<LogInfo>;

class LSSConfigMgr : public NonCopyable {
public:
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

    LogInfoPtr &getLogInfo();

  private:
    std::string name_{};
    int32_t cpu_start_{0};
    int32_t thread_nums_{1};

    bool parseLogInfo(const Json::Value &root);
    LogInfoPtr log_info_{};
  };

public:
  using LSSConfigPtr = std::shared_ptr<LSSConfig>;

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
