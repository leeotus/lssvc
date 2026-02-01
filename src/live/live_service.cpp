#include "live/live_service.h"
#include "live/base/live_logger.h"
#include "live/live_session.h"
#include "live/live_stream.h"
#include "mmedia/http/http_context.h"
#include "mmedia/http/http_request.h"
#include "mmedia/http/http_server.h"
#include "mmedia/http/http_utils.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "mmedia/rtmp/rtmp_server.h"
#include "network/base/lssvc_inetaddress.h"
#include "utils/lssvc_config.h"
#include "utils/lssvc_string.h"
#include "utils/lssvc_task.h"
#include "utils/lssvc_time.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>

using namespace lssvc::utils;
using namespace lssvc::mmedia;
using namespace lssvc::network;
using namespace lssvc::live;

namespace {
  static SessionPtr session_null;
}

SessionPtr LiveService::createSession(const std::string &session_name) {
  std::lock_guard<std::mutex> lk(lock_);
  auto it = sessions_.find(session_name);
  if(it != sessions_.end()) {
    return it->second;
  }
  auto list = utils::LSSString::split(session_name, "/");
  if(list.size() != 3) {
    LIVE_ERROR << "create session failed. Invalid session name:" << session_name;
    return session_null;
  }
  LSSConfigPtr config = g_config_mgr->getConfig();
  auto app_info = config->getAppInfo(list[0], list[1]);
  if (!app_info) {
    LIVE_ERROR << "create session failed. cant found config. domain:" << list[0]
               << " app:" << list[1];
    return session_null;
  }
  auto s = std::make_shared<LiveSession>(session_name);
  s->setAppInfo(app_info);

  sessions_.emplace(session_name, s);
  LIVE_DEBUG << "create session success. session_name:" << session_name
             << " now:" << LSSTime::nowMs();
  return s;
}

SessionPtr LiveService::findSession(const std::string &session_name) {
  std::lock_guard<std::mutex> lk(lock_);
  auto it = sessions_.find(session_name);
  if(it != sessions_.end()) {
    return it->second;
  }
  return session_null;
}

bool LiveService::closeSession(const std::string &session_name) {
  SessionPtr s;
  {
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = sessions_.find(session_name);
    if (iter != sessions_.end()) {
      s = iter->second;
      sessions_.erase(iter);
    }
  }
  if (s) {
    LIVE_INFO << " close session:" << s->getSessionName()
              << " now:" << LSSTime::nowMs();
    s->clear();
  }
  return true;
}

void LiveService::onTimer(const utils::LSSTaskPtr &t) {
  std::lock_guard<std::mutex> lk(lock_);
  for (auto iter = sessions_.begin(); iter != sessions_.end();) {
    if (iter->second->isTimeout()) {
      LIVE_INFO << "session:" << iter->second->getSessionName()
                << " is timeout. close it. Now:" << LSSTime::nowMs();
      iter->second->clear();
      iter = sessions_.erase(iter);
    } else {
      iter++;
    }
  }
  t->restart();
}

void LiveService::onNewConnection(const TcpConnectionPtr &conn) {}

void LiveService::onConnectionDestroy(
    const TcpConnectionPtr &conn) {
  auto user = conn->getContext<LiveUser>(kUserContext);
  if(user) {
    user->getSession()->closeUser(user);
  }
}

void LiveService::onActive(const ConnectionPtr &conn) {
  auto user = conn->getContext<PlayerUser>(kUserContext);
  if (user && user->getUserType() >= UserType::kUserTypePlayerPav) {
    user->postFrames();
  }
  // else {
  //   LIVE_ERROR << "no user found.host:" << conn->getPeerAddr().toIpWithPort();
  //   conn->forceClose();
  // }
}

bool LiveService::onPlay(const TcpConnectionPtr &conn,
                         const std::string &session_name,
                         const std::string &param) {
  LIVE_DEBUG << "on play session name:" << session_name << " param:" << param
             << " host:" << conn->getPeerAddr().toIpWithPort()
             << " now:" << LSSTime::nowMs();
  auto s = createSession(session_name);
  if (!s) {
    LIVE_ERROR << "create session failed.session name:" << session_name;
    conn->forceClose();
    return false;
  }
  auto user = s->createPlayerUser(conn, session_name, param,
                                  UserType::kUserTypePlayerRtmp);
  if (!user) {
    LIVE_ERROR << "create user failed.session name:" << session_name;
    conn->forceClose();
    return false;
  }
  conn->setContext(kUserContext, user);
  s->addPlayer(std::dynamic_pointer_cast<PlayerUser>(user));
  return true;
}

bool LiveService::onPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param)
{
  LIVE_DEBUG << "on publish session name:" << session_name << " param:" << param
             << " host:" << conn->getPeerAddr().toIpWithPort()
             << " now:" << LSSTime::nowMs();
  auto s = createSession(session_name);
  if (!s) {
    LIVE_ERROR << "create session failed.session name:" << session_name;
    conn->forceClose();
    return false;
    }
    auto user = s->createPublisher(conn,session_name,param,UserType::kUserTypePublishRtmp);
    if(!user)
    {
        LIVE_ERROR << "create user failed.session name:" << session_name;
        conn->forceClose();
        return false;
    }
    conn->setContext(kUserContext,user);
    s->setPublisher(user);
    return true;
}

void LiveService::onRecv(const TcpConnectionPtr &conn ,PacketPtr &&data)
{
    auto user = conn->getContext<LiveUser>(kUserContext);
    if(!user)
    {
        LIVE_ERROR << "no found user. host:" << conn->getPeerAddr().toIpWithPort();
        conn->forceClose();
        return ;
    }
    user->getStream()->addPacket(std::move(data));
}

void LiveService::onRecv(const TcpConnectionPtr &conn, const PacketPtr &data) {}

void LiveService::start() {
  LSSConfigPtr config = g_config_mgr->getConfig();
  pool_ = new LSSEventLoopThreadPool(config->thread_nums_, config->cpu_start_,
                                     config->cpus_);
  pool_->start();

  auto services = config->getServiceInfos();
  auto eventloops = pool_->getLoops();
  LIVE_TRACE << "eventloops' size:" << eventloops.size();
  for (auto &e : eventloops) {
    for (auto &s : services) {
      if (s->protocol == "rtmp" || s->protocol == "RTMP") {
        LSSInetAddress addr(s->addr, s->port); // default ipv4
        TcpServer *server = new RtmpServer(e, addr, this);
        servers_.push_back(server);
        servers_.back()->start();
      } else if(s->protocol == "http" || s->protocol == "HTTP") {
        LSSInetAddress addr(s->addr, s->port); // default ipv4
        TcpServer *server = new HttpServer(e, addr, this);
        servers_.push_back(server);
        servers_.back()->start();
      }
      // TODO: other protocols
    }
  }
  LSSTaskPtr t = std::make_shared<LSSTask>(
      std::bind(&LiveService::onTimer, this, std::placeholders::_1), 5000);
  g_task_mgr->add(t);
}

void LiveService::stop() {}

LSSEventLoop *LiveService::getNextLoop() {
  return pool_->getNextLoop();
}

void LiveService::onSend(const TcpConnectionPtr &conn) {}

bool LiveService::onSendNextChunk(const TcpConnectionPtr &conn) {
  return false;
}

// NOTE: to test http server:
// > curl -i -vo /dev/null "http://192.168.186.132:8080/live/test.flv"
// > wget "http://192.168.186.132:8080/live/test.flv"
// then you can use md5sum to verify these two video files
// > md5sum test.flv
void LiveService::onRequest(const TcpConnectionPtr &conn,
                            const HttpRequestPtr &req,
                            const PacketPtr &packet) {
  if (req->isRequest()) {
    LIVE_DEBUG << "request method:" << req->getMethod()
               << " path:" << req->getPath();
  } else {
    LIVE_DEBUG << "request code:" << req->getStatusCode() << " message:"
               << HttpUtils::parseStatusCode(req->getStatusCode());
  }

  auto headers = req->getHeaders();
  for(auto const &h : headers) {
    LIVE_DEBUG << h.first << ":" << h.second;
  }

  if(req->isRequest()) {
    // RESEARCH: DEBUG: open a specified file
    int fd = ::open("../test.flv", O_RDONLY, 0644);
    if(fd < 0) {
      LIVE_ERROR << "open file failed: " << strerror(errno);
      conn->forceClose();
      return;
    }
    HttpRequestPtr res = std::make_shared<HttpRequest>(false);
    res->setStatusCode(200); // success
    res->addHeader("server", "lssvc");
    res->addHeader("content-type", "video/x-flv");

    /* RESEARCH: DEBUG: for simple http request
    res->setStatusCode(200);
    res->addHeader("server", "lssvc");
    res->addHeader("content-type", "text/json");
    res->addHeader("content-length", std::to_string(strlen("{\"hello world\"}")));
    res->setBody("{\"{hello world}\"}");
    */

    auto ctx = conn->getContext<HttpContext>(kHttpContext);
    if(ctx) {
      res->setIsStream(true);
      ctx->postRequest(res);
    }

    while(true) {
      PacketPtr ndata = Packet::newPacket(65535);
      auto ret = ::read(fd, ndata->data(), 65535);
      if(ret <= 0) {
        break;
      }
      ndata->setPacketSize(ret);
      while(true) {
        auto sent = ctx->postStreamChunk(ndata);
        if(sent) {
          break;
        }
      }
    }
    ::close(fd);
    conn->forceClose();
  }
}
