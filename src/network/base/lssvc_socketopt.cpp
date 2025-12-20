#include "network/base/lssvc_socketopt.h"
#include "network/base/lssvc_netlogger.h"
#include <fcntl.h>
#include <sys/socket.h>

using namespace lssvc::network;

LSSocketOpt::LSSocketOpt(int sock, bool ipv6) : sock_fd_(sock), ipv6_(ipv6) {}

void LSSocketOpt::setNonblocking() {
  int flags = ::fcntl(sock_fd_, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sock_fd_, F_SETFL, flags);

  flags = ::fcntl(sock_fd_, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sock_fd_, F_SETFD, flags);
  (void)ret;
}

int LSSocketOpt::createNonblockingTcpSocket(int family) {
  int fd =
      ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (fd < 0) {
    NETWORK_ERROR << "Failed to create a nonblocking tcp socket\r\n";
  }
  return fd;
}

int LSSocketOpt::createNonblockingUdpSocket(int family) {
  int fd =
      ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
  if (fd < 0) {
    NETWORK_ERROR << "Failed to create a nonblocking udp socket\r\n";
  }
  return fd;
}

int LSSocketOpt::bindAddress(const LSSInetAddress &localaddr) {
  if (localaddr.isIpv6()) {
    // ipv6
    struct sockaddr_in6 addr;
    localaddr.getSockAddr((struct sockaddr *)&addr);
    socklen_t size = sizeof(struct sockaddr_in6);
    return ::bind(sock_fd_, (struct sockaddr *)&addr, size);
  } else {
    // ipv4
    struct sockaddr_in addr;
    localaddr.getSockAddr((struct sockaddr *)&addr);
    socklen_t size = sizeof(struct sockaddr_in);
    return ::bind(sock_fd_, (struct sockaddr *)&addr, size);
  }
  return 0;
}

int LSSocketOpt::listen() { return ::listen(sock_fd_, SOMAXCONN); }

int LSSocketOpt::accept(LSSInetAddress *peeraddr) {
  struct sockaddr_in6 addr;
  socklen_t len = sizeof(struct sockaddr_in6);
  int sock = ::accept4(sock_fd_, (struct sockaddr *)&addr, &len,
                       SOCK_CLOEXEC | SOCK_NONBLOCK);
  if (sock < 0) {
    NETWORK_ERROR << "Failed to accept client socket.\r\n";
    return -1;
  }
  if (addr.sin6_family == AF_INET6) {
    // ipv6
    char ip[INET6_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET6, &(addr.sin6_addr), ip, sizeof(ip));
    peeraddr->setAddr(ip);
    peeraddr->setPort(ntohs(addr.sin6_port));
    peeraddr->setIsIpv6(true);
  } else if (addr.sin6_family == AF_INET) {
    // ipv4
    char ip[16] = {0};
    struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
    ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
    peeraddr->setAddr(ip);
    peeraddr->setPort(ntohs(saddr->sin_port));
  }
  return sock;
}

int LSSocketOpt::connect(const LSSInetAddress &addr) {
  struct sockaddr_in6 addr_in;
  addr.getSockAddr((struct sockaddr *)&addr_in);
  return ::connect(sock_fd_, (struct sockaddr *)&addr_in,
                   sizeof(struct sockaddr_in6));
}

LSSInetAddressPtr LSSocketOpt::getLocalAddr() {
  struct sockaddr_in6 addr_in;
  socklen_t len = sizeof(struct sockaddr_in6);
  ::getsockname(sock_fd_, (struct sockaddr *)&addr_in, &len);
  LSSInetAddressPtr peeraddr = std::make_shared<LSSInetAddress>();
  if (addr_in.sin6_family == AF_INET) {
    char ip[16] = {0};
    struct sockaddr_in *saddr = (struct sockaddr_in *)&addr_in;
    ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
    peeraddr->setAddr(ip);
    peeraddr->setPort(ntohs(saddr->sin_port));
  } else if (addr_in.sin6_family == AF_INET6) {
    char ip[INET6_ADDRSTRLEN] = {
        0,
    };
    ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));
    peeraddr->setAddr(ip);
    peeraddr->setPort(ntohs(addr_in.sin6_port));
    peeraddr->setIsIpv6(true);
  }
  return peeraddr;
}

LSSInetAddressPtr LSSocketOpt::getPeerAddr() {
  struct sockaddr_in6 addr_in;
  socklen_t len = sizeof(struct sockaddr_in6);
  ::getpeername(sock_fd_, (struct sockaddr *)&addr_in, &len);
  LSSInetAddressPtr peeraddr = std::make_shared<LSSInetAddress>();
  if (addr_in.sin6_family == AF_INET) {
    char ip[16] = {
        0,
    };
    struct sockaddr_in *saddr = (struct sockaddr_in *)&addr_in;
    ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
    peeraddr->setAddr(ip);
    peeraddr->setPort(ntohs(saddr->sin_port));
  } else if (addr_in.sin6_family == AF_INET6) {
    char ip[INET6_ADDRSTRLEN] = {
        0,
    };
    ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));
    peeraddr->setAddr(ip);
    peeraddr->setPort(ntohs(addr_in.sin6_port));
    peeraddr->setIsIpv6(true);
  }

  return peeraddr;
}

void LSSocketOpt::setTcpNoDelay(bool on) {
  int v = on ? 1 : 0;
  ::setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
}

void LSSocketOpt::setReuseAddr(bool on) {
  int v = on ? 1 : 0;
  ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
}

void LSSocketOpt::setReusePort(bool on) {
  int v = on ? 1 : 0;
  ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(v));
}

void LSSocketOpt::setKeepAlive(bool on) {
  int v = on ? 1 : 0;
  ::setsockopt(sock_fd_, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v));
}
