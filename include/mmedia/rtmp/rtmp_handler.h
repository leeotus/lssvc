#ifndef __RTMP_HANDLER_H__
#define __RTMP_HANDLER_H__

#include "mmedia/base/mmedia_handler.h"
#include <string>

namespace lssvc::mmedia {

class RtmpHandler : public MMediaHandler {
public:
  virtual bool onPlay(const network::TcpConnectionPtr &conn,
                      const std::string &session_name,
                      const std::string &param) {
    return false;
  }

  virtual bool onPublish(const network::TcpConnectionPtr &conn,
                         const std::string &session_name,
                         const std::string &param) {
    return false;
  }

  virtual void onPause(const network::TcpConnectionPtr &conn, bool pause) = 0;

  virtual void onSeek(const network::TcpConnectionPtr &conn, double time) = 0;

  virtual void onPublishPrepare(const network::TcpConnectionPtr &conn) = 0;
};

} // namespace lssvc::mmedia

#endif
