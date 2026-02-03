#include "mmedia/http/http_server.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/flv/flv_context.h"
#include "mmedia/http/http_context.h"
#include <memory>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;

using HttpContextPtr = std::shared_ptr<HttpContext>;
using FlvContextPtr = std::shared_ptr<FlvContext>;

HttpServer::HttpServer(network::LSSEventLoop *loop,
                       const network::LSSInetAddress &local,
                       HttpHandler *handler)
    : TcpServer(loop, local), http_handler_(handler) {}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
  HTTP_DEBUG << "HttpServer started";
  TcpServer::setActiveCallback(
      std::bind(&HttpServer::onActive, this, std::placeholders::_1));
  TcpServer::setDestroyConnectionCallback(
      std::bind(&HttpServer::onDestroyed, this, std::placeholders::_1));
  TcpServer::setNewConnectionCallback(
      std::bind(&HttpServer::onNewConnection, this, std::placeholders::_1));
  TcpServer::setWriteCompleteCallback(
      std::bind(&HttpServer::onWriteComplete, this, std::placeholders::_1));
  TcpServer::setMessageCallback(std::bind(&HttpServer::onMessage, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
  TcpServer::start();
}

void HttpServer::stop() { TcpServer::stop(); }

void HttpServer::onNewConnection(const TcpConnectionPtr &conn) {
  if(http_handler_) {
    // processed by the upper level
    http_handler_->onNewConnection(conn);
  }
  HttpContextPtr ctx = std::make_shared<HttpContext>(conn, http_handler_);
  conn->setContext(kHttpContext, ctx);
}

void HttpServer::onDestroyed(const TcpConnectionPtr &conn) {
  if(http_handler_) {
    http_handler_->onConnectionDestroy(conn);
  }
  conn->clearContext(kHttpContext);
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, LSSMsgBuffer &buf) {
  HttpContextPtr ctx = conn->getContext<HttpContext>(kHttpContext);
  if(ctx) {
    int ret = ctx->parse(buf);
    if(ret == -1) {
      conn->forceClose();
    }
  }
}

void HttpServer::onWriteComplete(const ConnectionPtr &conn) {
  HttpContextPtr http_ctx = conn->getContext<HttpContext>(kHttpContext);
  if (http_ctx) {
    http_ctx->writeComplete(std::dynamic_pointer_cast<LSSTcpConnection>(conn));
  }
  FlvContextPtr flv_ctx = conn->getContext<FlvContext>(kFlvContext);
  if (flv_ctx) {
    flv_ctx->writeComplete(std::dynamic_pointer_cast<LSSTcpConnection>(conn));
  }
}

void HttpServer::onActive(const ConnectionPtr &conn) {
  if(http_handler_) {
    http_handler_->onActive(conn);
  }
}
