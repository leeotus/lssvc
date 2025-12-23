#ifndef __LSSVC_SOCKETOPT_H__
#define __LSSVC_SOCKETOPT_H__

#include "lssvc_inetaddress.h"
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace lssvc::network {

using LSSInetAddressPtr = std::shared_ptr<LSSInetAddress>;

class LSSocketOpt {
public:
  LSSocketOpt(int sock, bool ipv6 = false);
  ~LSSocketOpt() = default;

  /**
   * @brief create a nonblocking tcp socket
   * @param family [in] socket family type
   * @return int return the tcp file description
   */
  static int createNonblockingTcpSocket(int family);

  /**
   * @brief create a nonblocking udp socket
   * @param family [in] socket family type
   * @return int return the udp file description
   */
  static int createNonblockingUdpSocket(int family);

  // @brief set socket non-blocking
  void setNonblocking();

  /**
   * @brief bind address to the socket
   * @param localaddr [in] stores the ip address and the corresponing port
   * @return int return 0 if success
   */
  int bindAddress(const LSSInetAddress &localaddr);

  int listen();

  /**
   * @brief accept new connections and parse the address information
   * of the client (peer end)
   * @param peeraddr [out] store the incoming client's address and port
   * @return the client fd
   */
  int accept(LSSInetAddress *peeraddr);

  /**
   * @brief initiate a tcp connection to the target address
   * @param addr [in] the target address
   */
  int connect(const LSSInetAddress &addr);

  // @brief obtain the current (server) socket's address
  LSSInetAddressPtr getLocalAddr();

  // @brief obtain the peer address connected to the current socket
  LSSInetAddressPtr getPeerAddr();

  void setTcpNoDelay(bool on = true);

  void setReuseAddr(bool on = true);

  /**
   * @brief control whether multiple processes are allowed to
   * bind to the same port
   * @param on [in] true if allow
   */
  void setReusePort(bool on = true);

  // @brief control whether to enable the TCP keep-alive or not
  void setKeepAlive(bool on = true);

private:
  int sock_fd_;
  bool ipv6_;
};

} // namespace lssvc::network

#endif
