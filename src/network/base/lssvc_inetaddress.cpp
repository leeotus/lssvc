#include "network/base/lssvc_inetaddress.h"
#include "network/base/lssvc_netlogger.h"
#include "utils/lssvc_string.h"

#include <sstream>
#include <string.h>
#include <string>

using namespace lssvc::network;

int LSSInetAddress::getIpAndPort(const std::string &host, std::string &ip,
                                 std::string &port) {
  std::string delim{":"};
  auto ret = lssvc::utils::LSSString::split(host, delim);
  if (ret.size() == 1) {
    // @todo need the validity of the result string (string must be an ip address)
    NETWORK_WARN << "Failed to split host address\r\n";
    ip = ret[0];
    port = "0";
    return -1;
  } else if (ret.size() == 2) {
    ip = ret[0];
    port = ret[1];
    return 0;
  }
  NETWORK_ERROR << "No input ip address and port, please check.\r\n";
  return -1;
}

LSSInetAddress::LSSInetAddress(const std::string &ip, uint16_t port, bool ipv6)
    : addr_(ip), port_(std::to_string(port)), is_ipv6_(ipv6) {}

LSSInetAddress::LSSInetAddress(const std::string &host, bool ipv6) {
  getIpAndPort(host, addr_, port_);
  is_ipv6_ = is_ipv6_;
}

void LSSInetAddress::setHost(const std::string &host) {
  getIpAndPort(host, addr_, port_);
}

void LSSInetAddress::setAddr(const std::string &addr) { this->addr_ = addr; }

void LSSInetAddress::setPort(const uint16_t port) {
  this->port_ = std::to_string(port);
}

void LSSInetAddress::setIsIpv6(bool ipv6) { this->is_ipv6_ = ipv6; }

uint32_t LSSInetAddress::getIpv4() const { return IPv4(this->addr_.c_str()); }

const std::string &LSSInetAddress::getIp() const { return addr_; }

std::string LSSInetAddress::toIpWithPort() const {
  std::stringstream ss;
  ss << addr_ << ":" << port_;
  return ss.str();
}

uint16_t LSSInetAddress::getPort() const { return std::atoi(port_.c_str()); }

void LSSInetAddress::getSockAddr(struct sockaddr *saddr) const {
  if (is_ipv6_) {
    struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)saddr;
    memset(addr_in6, 0, sizeof(struct sockaddr_in6));
    addr_in6->sin6_family = AF_INET6;
    addr_in6->sin6_port = htons(std::atoi(port_.c_str()));
    if (::inet_pton(AF_INET6, addr_.c_str(), &addr_in6->sin6_addr) < 0) {
    }
  } else {
    struct sockaddr_in *addr_in = (struct sockaddr_in *)saddr;
    memset(addr_in, 0, sizeof(struct sockaddr_in));
    addr_in->sin_family = AF_INET;
    addr_in->sin_port = htons(std::atoi(port_.c_str()));
    if (::inet_pton(AF_INET, addr_.c_str(), &addr_in->sin_addr) < 0) {
    }
  }
}

bool LSSInetAddress::isIpv6() const { return is_ipv6_; }

bool LSSInetAddress::isWanIp() const {
  uint32_t a_start = IPv4("10.0.0.0");
  uint32_t a_end = IPv4("10.255.255.255");
  uint32_t b_start = IPv4("172.16.0.0");
  uint32_t b_end = IPv4("172.31.255.255");
  uint32_t c_start = IPv4("192.168.0.0");
  uint32_t c_end = IPv4("192.168.255.255");
  uint32_t ip = getIpv4();
  bool is_a = ip >= a_start && ip <= a_end;
  bool is_b = ip >= b_start && ip <= b_end;
  bool is_c = ip >= c_start && ip <= c_end;

  return !is_a && !is_b && !is_c && ip != INADDR_LOOPBACK;
}

bool LSSInetAddress::isLanIp() const {
  uint32_t a_start = IPv4("10.0.0.0");
  uint32_t a_end = IPv4("10.255.255.255");
  uint32_t b_start = IPv4("172.16.0.0");
  uint32_t b_end = IPv4("172.31.255.255");
  uint32_t c_start = IPv4("192.168.0.0");
  uint32_t c_end = IPv4("192.168.255.255");
  uint32_t ip = getIpv4();
  bool is_a = ip >= a_start && ip <= a_end;
  bool is_b = ip >= b_start && ip <= b_end;
  bool is_c = ip >= c_start && ip <= c_end;
  return is_a || is_b || is_c;
}

bool LSSInetAddress::isLoopbackIp() const { return addr_ == "127.0.0.1"; }

uint32_t LSSInetAddress::IPv4(const char *ip) const {
  struct sockaddr_in addr_in;
  memset(&addr_in, 0, sizeof(struct sockaddr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = 0;
  if (::inet_pton(AF_INET, ip, &addr_in.sin_addr) < 0) {
    NETWORK_ERROR << "ipv4 ip: " << ip << "convert failed.\r\n";
  }

  // convert network byte order to host byte one for the
  // comparison of IP address on the local host
  return ntohl(addr_in.sin_addr.s_addr);
}
