#include "network/dns_service.h"
#include "network/base/lssvc_netlogger.h"
#include <functional>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace lssvc::network;

namespace lssvc::network {
static InetAddressPtr inetaddress_null = nullptr;
}

DnsService::~DnsService() {}

void DnsService::addHost(const std::string &host) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = hosts_info_.find(host);
  if (it != hosts_info_.end()) {
    // already in the hosts_info
    return;
  }
  hosts_info_[host] = std::vector<InetAddressPtr>{};
}

InetAddressPtr DnsService::getHostAddress(const std::string &host, int index) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = hosts_info_.find(host);
  if (it == hosts_info_.end()) {
    return inetaddress_null;
  }
  std::vector<InetAddressPtr> addr_list = it->second;
  if (addr_list.size() > 0) {
    return addr_list[index % addr_list.size()];
  }
  return inetaddress_null;
}

std::vector<InetAddressPtr>
DnsService::getHostAddresses(const std::string &host) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = hosts_info_.find(host);
  if (it != hosts_info_.end()) {
    return it->second;
  }
  return std::vector<InetAddressPtr>{};
}

std::unordered_map<std::string, std::vector<InetAddressPtr>>
DnsService::getHosts() {
  std::lock_guard<std::mutex> lock(lock_);
  return hosts_info_;
}

void DnsService::updateHost(const std::string &host,
                            std::vector<InetAddressPtr> &list) {
  std::lock_guard<std::mutex> lock(lock_);
  hosts_info_[host].swap(list);
}

void DnsService::setDnsServiceParam(int32_t interval, int32_t sleep,
                                    int32_t retry) {
  interval_ = interval;
  sleep_ = sleep;
  retry_ = retry;
}

void DnsService::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&DnsService::onWork, this));
}

void DnsService::stop() {
  running_ = false;
  {
    std::unique_lock<std::mutex> lock(stop_lock_);
    // stop the DNS service
    stop_cond_.notify_one();
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void DnsService::onWork() {
  while (running_) {
    auto host_infos = getHosts();
    for (auto &host : host_infos) {
      for (int i = 0; i < retry_; ++i) {
        std::vector<InetAddressPtr> list;
        getHostInfo(host.first, list);
        if (list.size() > 0) {
          updateHost(host.first, list);
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_));
      }
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
    std::unique_lock<std::mutex> lock(stop_lock_);
    stop_cond_.wait_for(lock, std::chrono::milliseconds(interval_));
  }
}

void DnsService::getHostInfo(const std::string &host,
                             std::vector<InetAddressPtr> &list) {
  struct addrinfo addr_info, *res;
  memset(&addr_info, 0, sizeof(struct addrinfo));
  addr_info.ai_family = AF_UNSPEC;
  addr_info.ai_flags = AI_PASSIVE;
  addr_info.ai_socktype = SOCK_DGRAM; // use UDP
  int ret = ::getaddrinfo(host.c_str(), nullptr, &addr_info, &res);
  if (ret == -1 || res == nullptr) {
    return;
  }
  struct addrinfo *rp = res;
  for (; rp != nullptr; rp = rp->ai_next) {
    InetAddressPtr peeraddr = std::make_shared<LSSInetAddress>();
    if (rp->ai_family == AF_INET) {
      char ip[16] = {0};
      struct sockaddr_in *saddr = (struct sockaddr_in *)rp->ai_addr;
      ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
      peeraddr->setAddr(ip);
      peeraddr->setPort(ntohs(saddr->sin_port));
      peeraddr->setIsIpv6(false);
    } else if (rp->ai_family == AF_INET6) {
      char ip[INET6_ADDRSTRLEN] = {0};
      struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)rp->ai_addr;
      ::inet_ntop(AF_INET6, &(saddr->sin6_addr), ip, sizeof(ip));
      peeraddr->setAddr(ip);
      peeraddr->setPort(ntohs(saddr->sin6_port));
      peeraddr->setIsIpv6(true);
    }
    list.push_back(peeraddr);
  }
}
