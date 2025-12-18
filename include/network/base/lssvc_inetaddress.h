#ifndef __LSSVC_INETADDRESS_H__
#define __LSSVC_INETADDRESS_H__

#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <bits/socket.h>
#include <netinet/in.h>

namespace lssvc::network {

class LSSInetAddress {
public:
  /**
   * @brief construct a new LSSInetAddress object
   * @param ip [in] ip address
   * @param port [in] port
   * @param ipv6 [in] whether ip is ipv6 version or not
   */
  LSSInetAddress(const std::string &ip, uint16_t port, bool ipv6 = false);

  // @note host: ip+port
  LSSInetAddress(const std::string &host, bool ipv6 = false);

  LSSInetAddress() = default;
  ~LSSInetAddress() = default;

  // setters
  void setHost(const std::string &host);
  void setAddr(const std::string &addr);
  void setPort(const uint16_t port);
  void setIsIpv6(bool ipv6);

  // getters
  uint32_t getIpv4() const;
  const std::string &getIp() const;
  std::string toIpWithPort() const;
  uint16_t getPort() const;
  void getSockAddr(struct sockaddr *saddrr) const;

  /**
   * @brief split a host address (ip + port), and return its ip and port part
   * @param host [in] input host address
   * @param ip [out] ip part of host
   * @param port [out] port part of host
   * @return 0 if success, else -1
   */
  static int getIpAndPort(const std::string &host, std::string &ip,
                          std::string &port);

  bool isIpv6() const;
  bool isWanIp() const;
  bool isLanIp() const;
  bool isLoopbackIp() const;

private:
  // @brief return the local host byte order of the input ip
  uint32_t IPv4(const char *ip) const;

  std::string addr_;
  std::string port_;
  bool is_ipv6_{false};
};

} // namespace lssvc::network

#endif
