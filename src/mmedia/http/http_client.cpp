#include "mmedia/http/http_client.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/http/http_context.h"
#include "network/dns_service.h"

using namespace lssvc::network;
using namespace lssvc::mmedia;

HttpClient::HttpClient(LSSEventLoop *loop, HttpHandler *handler)
    : loop_(loop), handler_(handler) {}

HttpClient::~HttpClient() {}

void HttpClient::onWriteComplete(const network::TcpConnectionPtr &conn) {
  auto ctx = conn->getContext<HttpContext>(kHttpContext);
  if(ctx) {
    ctx->writeComplete(conn);
  }
}

void HttpClient::onConnection(const TcpConnectionPtr &conn,
                              bool connected) {
  if (connected) {
    auto ctx = std::make_shared<HttpContext>(conn, handler_);
    conn->setContext(kHttpContext, ctx);
    if (is_post_) {
      ctx->postRequest(request_->makeHeaders(), out_packet_);
    } else {
      ctx->postRequest(request_->makeHeaders());
    }
  }
}

void HttpClient::onMessage(const network::TcpConnectionPtr &conn,
                           network::LSSMsgBuffer &buf) {
  auto ctx = conn->getContext<HttpContext>(kHttpContext);
  if (ctx) {
    auto ret = ctx->parse(buf);
    if (ret == -1) {
      RTMP_ERROR << "message parsed failed";
      conn->forceClose();
    }
  }
}

bool HttpClient::parseUrl(const std::string &url) {
  if(url.size() > 7) {
    // http://
    uint16_t port = 80;
    auto pos = url.find_first_of("/", 7);
    if(pos != std::string::npos) {
      const std::string &path = url.substr(pos);
      auto pos1 = path.find_first_of("?");
      if(pos1 != std::string::npos) {
        request_->setPath(path.substr(0, pos1));
        request_->setQuery(path.substr(pos1 + 1));
      } else {
        request_->setPath(path);
      }
      std::string domain = url.substr(7, pos - 7);
      request_->addHeader("Host", domain);
      auto pos2 = domain.find_first_of(":");
      if(pos2 != std::string::npos) {
        addr_.setAddr(domain.substr(0, pos2));
        addr_.setPort(std::atoi(url.substr(pos2 + 1).c_str()));
      } else {
        addr_.setAddr(domain);
        addr_.setPort(port);
      }
    } else {
      request_->setPath("/");
      std::string domain = url.substr(7);
      request_->addHeader("Host", domain);
      addr_.setAddr(domain);
      addr_.setPort(port);
    }
    auto list = g_dns_service->getHostAddresses(addr_.getIp());
    if(list.size() > 0) {
      for(auto const &it : list) {
        if (!it->isIpv6()) {
          addr_.setAddr(it->getIp());
          break;
        }
        // TODO: ipv6
      }
    } else {
      g_dns_service->addHost(addr_.getIp());
      std::vector<InetAddressPtr> list2;
      g_dns_service->getHostInfo(addr_.getIp(), list2);
      if(list2.size() > 0) {
        for(auto const &it : list2) {
          if(!it->isIpv6()) {
            addr_.setAddr(it->getIp());
            break;
          }
          // TODO: ipv6
        }
      }
    }
    return true;
  }
  return false;
}

void HttpClient::createTcpClient() {
  request_.reset();
  request_ = std::make_shared<HttpRequest>(true);
  if (is_post_) {
    request_->setMethod(kPost);
  } else {
    request_->setMethod(kGet);
  }
  request_->addHeader("User-Agent", "curl/7.61.1");
  request_->addHeader("Accept", "*/*");
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
      std::bind(&HttpClient::onWriteComplete, this, std::placeholders::_1));
  tcp_client_->setRecvMsgCallback(std::bind(&HttpClient::onMessage, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));
  tcp_client_->setCloseCallback(close_cb_);
  tcp_client_->setConnectCallback(std::bind(&HttpClient::onConnection, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));
  tcp_client_->connect();
}

void HttpClient::get(const std::string &url) {
  is_post_ = false;
  url_ = url;
  createTcpClient();
}

void HttpClient::post(const std::string &url, const PacketPtr &pkt) {
  is_post_ = true;
  url_ = url;
  out_packet_ = pkt;
  createTcpClient();
}
