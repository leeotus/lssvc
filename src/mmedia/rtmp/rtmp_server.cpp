#include "mmedia/rtmp/rtmp_server.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/rtmp_handshake.h"
#include "mmedia/rtmp/rtmp_context.h"

using namespace lssvc::mmedia;
using namespace lssvc::utils;
using namespace lssvc::network;

RtmpServer::RtmpServer(network::LSSEventLoop *loop,
                       network::LSSInetAddress &local, RtmpHandler *handler)
    : TcpServer(loop, local), rtmp_handler_(handler) {}

RtmpServer::~RtmpServer() { stop(); }

void RtmpServer::start() {
  TcpServer::setActiveCallback(
      std::bind(&RtmpServer::onActive, this, std::placeholders::_1));

  TcpServer::setDestroyConnectionCallback(
      std::bind(&RtmpServer::onDestroyed, this, std::placeholders::_1));

  TcpServer::setNewConnectionCallback(
      std::bind(&RtmpServer::onNewConnection, this, std::placeholders::_1));

  TcpServer::setWriteCompleteCallback(
      std::bind(&RtmpServer::onWriteComplete, this, std::placeholders::_1));

  TcpServer::setMessageCallback(std::bind(&RtmpServer::onMessage, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));

  TcpServer::start();
}

void RtmpServer::stop() {
  TcpServer::stop();
}

void RtmpServer::onNewConnection(const network::TcpConnectionPtr &conn) {
  if(rtmp_handler_) {
    rtmp_handler_->onNewConnection(conn);
  }
  // send RTMP handshake packet to the incoming connection
  RtmpContextPtr ctx = std::make_shared<RtmpContext>(conn, nullptr, false);
  conn->setContext(kRtmpContext, ctx);
  ctx->startHandShake();
}

void RtmpServer::onDestroyed(const network::TcpConnectionPtr &conn) {
  if (rtmp_handler_) {
    rtmp_handler_->onConnectionDestroy(conn);
  }
  conn->clearContext();
}

void RtmpServer::onMessage(const network::TcpConnectionPtr &conn,
                           network::LSSMsgBuffer &buf) {
  RtmpContextPtr ctx = conn->getContext<RtmpContext>(kRtmpContext);
  if (ctx) {
    int ret = ctx->parse(buf);
    if (ret == 0) {
      RTMP_TRACE << "host: " << conn->getPeerAddr().toIpWithPort()
                 << " handshake success";
    } else if (ret == -1) {
      // failed
      conn->forceClose();
    }
  }
}

void RtmpServer::onWriteComplete(const network::ConnectionPtr &conn) {
  RtmpContextPtr ctx = conn->getContext<RtmpContext>(kRtmpContext);
  if (ctx) {
    ctx->onWriteComplete();
  }
}
void RtmpServer::onActive(const network::ConnectionPtr &conn) {
  if (rtmp_handler_) {
    rtmp_handler_->onActive(conn);
  }
}
