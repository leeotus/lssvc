#ifndef __LSSVC_DOMAIN_INFO_H__
#define __LSSVC_DOMAIN_INFO_H__

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/*
Example
{
  "domain":
  {
    "name": "domain.com",
    "type": "publish",

    "app":
    [
      ... @see appinfo
    ]
  }
}
*/

namespace lssvc::utils {

class LSSAppInfo;
using AppInfoPtr = std::shared_ptr<LSSAppInfo>;

class LSSDomainInfo {
public:
  LSSDomainInfo() = default;
  ~LSSDomainInfo() = default;

  // @brief get the domain name
  const std::string &getDomainName() const;

  // @brief get the value of "type" field
  const std::string &getType() const;

  /**
   * @brief parse the domain information from input file
   * @param filepath [in] path to the domain description file
   * @return true if success, else false
   */
  bool parseDomainInfo(const std::string &filepath);

  /**
   * @brief get the value of "app" feild from the parsed data
   * @param app_name [in] the name of the field
   */
  AppInfoPtr getAppInfo(const std::string &app_name);

private:
  std::string name_;
  std::string type_;
  std::mutex lock_;

  std::unordered_map<std::string, AppInfoPtr> appinfos_;
};

} // namespace lssvc::utils

#endif
