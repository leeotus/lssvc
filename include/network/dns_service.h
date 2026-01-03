#ifndef __DNS_SERVICE_H__
#define __DNS_SERVICE_H__

#include "base/lssvc_inetaddress.h"
#include "utils/noncopyable.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define g_dns_service                                                          \
  lssvc::utils::LSSSingleton<lssvc::network::DnsService>::getInstance()

namespace lssvc {

namespace network {

using InetAddressPtr = std::shared_ptr<LSSInetAddress>;

class DnsService : public utils::NonCopyable {
public:
  DnsService() = default;
  ~DnsService();

  /**
   * @brief add host to the hosts_info
   * @param host [in] host address
   */
  void addHost(const std::string &host);

  /**
   * @brief get the ip address of the host, default ip index equals to 0
   * @param host [in] host address
   * @param index [in] index of the resolved ip address
   * @return InetAddressPtr the ip address stored in the InetAddress object
   */
  InetAddressPtr getHostAddress(const std::string &host, int index = 0);

  /**
   * @brief get the corresponding ip address of the input host address
   * @param host [in] the input host address
   * @return std::vector<InetAddressPtr> the vector which stores the
   * corresponding resolved ip addresses
   */
  std::vector<InetAddressPtr> getHostAddresses(const std::string &host);

  // @brief return hosts_info
  std::unordered_map<std::string, std::vector<InetAddressPtr>> getHosts();

  /**
   * @brief update host infomations
   * @param host [in] the input host address
   * @param list [in] the updated ip addresses
   */
  void updateHost(const std::string &host, std::vector<InetAddressPtr> &list);

  void setDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry);

  // @brief start the DNS service
  void start();

  // @brief stop the DNS service
  void stop();

  // @brief DNS service's working thread
  void onWork();

  /**
   * @brief resolve domain names into Ip addresses
   * @param host [in] host address
   * @param list [out] the resolved ip addresses
   */
  static void getHostInfo(const std::string &host,
                          std::vector<InetAddressPtr> &list);

private:
  std::thread thread_;
  bool running_{false};
  std::mutex lock_;
  int32_t retry_{3};
  int32_t sleep_{200};           // ms
  int32_t interval_{180 * 1000}; // s

  std::mutex stop_lock_;
  std::condition_variable stop_cond_;

  std::unordered_map<std::string, std::vector<InetAddressPtr>> hosts_info_;
};

} // namespace network
} // namespace lssvc

#endif
