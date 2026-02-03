#ifndef __RTMP_HANDLER_H__
#define __RTMP_HANDLER_H__

#include "mmedia/base/mmedia_handler.h"
#include <string>

namespace lssvc::mmedia {

class RtmpHandler : virtual public MMediaHandler {
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

  virtual void onPause(const network::TcpConnectionPtr &conn, bool pause) {};

  virtual void onSeek(const network::TcpConnectionPtr &conn, double time) {};

  virtual void onPublishPrepare(const network::TcpConnectionPtr &conn) {};
};

} // namespace lssvc::mmedia

#endif
