#ifndef __LIVE_SERVICE_H__
#define __LIVE_SERVICE_H__

#include "mmedia/base/packet.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "network/net/lssvc_connection.h"
#include "network/net/lssvc_eventloop_threadpool.h"
#include "network/tcp_server.h"
#include "utils/lssvc_singleton.h"
#include "utils/lssvc_task.h"
#include "utils/lssvc_taskmgr.h"
#include "utils/noncopyable.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace lssvc::live {

class LiveSession;
using SessionPtr = std::shared_ptr<LiveSession>;

class LiveService : public mmedia::RtmpHandler {
public:
  LiveService() = default;
  ~LiveService() = default;

  // @brief create a session
  SessionPtr createSession(const std::string &session_name);

  // @brief find the session (if exists) according to the input session name
  SessionPtr findSession(const std::string &session_name);

  // @brief close the specific session
  bool closeSession(const std::string &session_name);

  void onTimer(const utils::LSSTaskPtr &t);

  void onNewConnection(const network::TcpConnectionPtr &conn) override;
  void onConnectionDestroy(const network::TcpConnectionPtr &conn) override;
  void onActive(const network::ConnectionPtr &conn) override;
  bool onPlay(const network::TcpConnectionPtr &conn, const std::string &session_name,
              const std::string &param) override;
  bool onPublish(const network::TcpConnectionPtr &conn, const std::string &session_name,
                 const std::string &param) override;
  void onRecv(const network::TcpConnectionPtr &conn, mmedia::PacketPtr &&data) override;
  void onRecv(const network::TcpConnectionPtr &conn, const mmedia::PacketPtr &data) override;

  // @brief start this service
  void start();

  // @brief stop this service
  void stop();

  network::LSSEventLoop *getNextLoop();
private:
  network::LSSEventLoopThreadPool *pool_{nullptr};
  std::vector<network::TcpServer*> servers_;
  std::mutex lock_;
  std::unordered_map<std::string, SessionPtr> sessions_;
};

#define gLiveService lssvc::utils::LSSSingleton<lssvc::live::LiveService>::getInstance()

}

#endif
