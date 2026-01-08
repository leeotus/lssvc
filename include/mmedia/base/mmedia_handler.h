#ifndef __MMEDIA_HANDLER_H__
#define __MMEDIA_HANDLER_H__

#include "network/net/lssvc_tcpconn.h"
#include "packet.h"
#include "utils/noncopyable.h"
#include <memory>

namespace lssvc {

namespace mmedia {

class MMediaHandler : public utils::NonCopyable {
public:
  /**
   * @brief handle new incoming connection
   * @param conn [in] incoming tcp connection
   */
  virtual void onNewConnection(const network::TcpConnectionPtr &conn) = 0;

  /**
   * @brief execute callback when a connection is destroyed
   * @param conn [in] the disconnected tcp connection
   */
  virtual void onConnectionDestroy(const network::TcpConnectionPtr &conn) = 0;

  /**
   * @brief the data parsed by the multimedia protocol needs to be passed to
   * the live streaming business module through this interface
   * @param conn [in] the peer tcp connection
   * @param data [in] the parsed data
   */
  virtual void onRecv(const network::TcpConnectionPtr &conn,
                      PacketPtr &data) = 0;
  virtual void onRecv(const network::TcpConnectionPtr &conn,
                      PacketPtr &&data) = 0;

  // @brief notify the live streaming business module
  virtual void onActive(const network::ConnectionPtr &conn) = 0;
};

} // namespace mmedia

} // namespace lssvc

#endif
