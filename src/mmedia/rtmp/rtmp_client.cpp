#include "mmedia/rtmp/rtmp_client.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/rtmp_context.h"

using namespace lssvc::mmedia;
using namespace lssvc::network;

RtmpClient::RtmpClient(LSSEventLoop *loop, RtmpHandler *handler)
    : loop_(loop), handler_(handler) {}

RtmpClient::~RtmpClient() {}

void RtmpClient::onWriteComplete(const network::TcpConnectionPtr &conn) {
  auto ctx = conn->getContext<RtmpContext>(kRtmpContext);
  if(ctx) {
    ctx->onWriteComplete();
  }
}

void RtmpClient::onConnection(const network::TcpConnectionPtr &conn, bool connected) {
  if(connected) {
    auto ctx = std::make_shared<RtmpContext>(conn, handler_, true);
    if(is_player_) {
      ctx->play(url_);
    } else {
      ctx->publish(url_);
    }
    conn->setContext(kRtmpContext, ctx);
    ctx->startHandShake();
  }
}

void RtmpClient::onMessage(const network::TcpConnectionPtr &conn, network::LSSMsgBuffer &buf) {
  auto ctx = conn->getContext<RtmpContext>(kRtmpContext);
  if(ctx) {
    auto ret = ctx->parse(buf);
    if(ret == -1) {
      RTMP_ERROR << "message parse error";
      conn->forceClose();
    }
    // TODO
  }
}

bool RtmpClient::parseUrl(const std::string &url) {
  if (url.size() > 7) // rtmp://
  {
    uint16_t port = 1935; // rtmp's port
    auto pos = url.find_first_of(":/", 7);
    if (pos != std::string::npos) {
      std::string domain = url.substr(7, pos - 7);
      if (url.at(pos) == ':') {
        auto pos1 = url.find_first_of("/", pos + 1);
        if (pos1 != std::string::npos) {
          port = std::atoi(url.substr(pos + 1, pos1 - pos).c_str());
        }
      }
      addr_.setAddr(domain);
      addr_.setPort(port);
      return true;
    }
  }
  return false;
}

void RtmpClient::play(const std::string &url) {
  is_player_ = true;
  url_ = url;
  createTcpClient();
}

void RtmpClient::publish(const std::string &url) {
  is_player_ = false;
  url_ = url;
  createTcpClient();
}

void RtmpClient::createTcpClient() {
  auto ret = parseUrl(url_);
  if (!ret) {
    RTMP_ERROR << "invalid url:" << url_;
    if (close_cb_) {
      close_cb_(nullptr);
    }
    return;
  }

  tcp_client_ = std::make_shared<TcpClient>(loop_, addr_);
  tcp_client_->setWriteCompleteCallback(
      std::bind(&RtmpClient::onWriteComplete, this, std::placeholders::_1));
  tcp_client_->setRecvMsgCallback(std::bind(&RtmpClient::onMessage, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));
  tcp_client_->setCloseCallback(close_cb_);
  tcp_client_->setConnectCallback(std::bind(&RtmpClient::onConnection, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));
  tcp_client_->connect();
}
