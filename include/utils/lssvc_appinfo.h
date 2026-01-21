#ifndef __LSSVC_APPINFO_H__
#define __LSSVC_APPINFO_H__

#include "json/json.h"
#include <cstdint>
#include <memory>
#include <string>

/*
Example
{
  "domain":
  {
    ... @see domain_info

    "app":
    [
      "name": "live",
      "max_buffer": 1000,
      "hls_support": "on",
      "flv_support": "on",
      "rtmp_suppot": "on",
      "content_latency": 3,
    ]
  }
}
*/

namespace lssvc::utils {

class LSSDomainInfo;

class LSSAppInfo {
public:
  explicit LSSAppInfo(LSSDomainInfo &domain);
  ~LSSAppInfo() = default;

  /**
   * @brief parse the "app" field to get data
   * @param root [in] the root of the json
   * @return true if success, else false
   */
  bool parseAppInfo(Json::Value &root);

  LSSDomainInfo &domain_info_;
  std::string domain_name_;
  std::string app_name_;

  uint32_t max_buffer_{1000};
  bool rtmp_support_{false};
  bool flv_support_{false};
  bool hls_support_{false};
  uint32_t content_latency_{3 * 1000};   // ms
  uint32_t stream_idle_time_{30 * 1000};
  uint32_t stream_timeout_time_{30 * 1000};
};

using AppInfoPtr = std::shared_ptr<LSSAppInfo>;

} // namespace lssvc::utils

#endif
