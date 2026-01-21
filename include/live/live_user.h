#ifndef __LIVE_USER_H__
#define __LIVE_USER_H__

#include "network/net/lssvc_connection.h"
#include "utils/lssvc_appinfo.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace lssvc::live {

class LiveSession;
using SessionPtr= std::shared_ptr<LiveSession>;

enum class UserType {
  kUserTypePublishRtmp = 0,
  kUserTypePublishMpegts,
  kUserTypePublishPav,
  kUserTypePublishWebRTC,
  kUserTypePlayerPav,
  kUserTypePlayerFlv,
  kUserTypePlayerHls,
  kUserTypePlayerRtmp,
  kUserTypePlayerWebRTC,
  kUserTypeUnknown = 255,
};

enum class UserProtocol {
  kUserProtocolHttp = 0,
  kUserProtocolHttps,
  kUserProtocolQuic,
  kUserProtocolRtsp,
  kUserProtocolWebRTC,
  kUserProtocolUdp,
  kUserProtocolUnknown = 255
};

class LiveStream;
using StreamPtr = std::shared_ptr<LiveStream>;

class LiveUser : public std::enable_shared_from_this<LiveUser> {
public:
  friend class LiveSession;

  explicit LiveUser(const network::ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);
  virtual ~LiveUser() = default;

  // @brief getter, return the domain name
  const std::string &getDomainName() const;
  // @brief setter, set the domain name
  void setDomainName(const std::string &domain);

  const std::string &getAppName() const;
  void setAppName(const std::string &domain);

  const std::string &getStreamName() const;
  void setStreamName(const std::string &domain);

  const std::string &getParam() const;
  void setParam(const std::string &domain);

  const utils::AppInfoPtr &getAppInfo() const;
  void setAppInfo(const utils::AppInfoPtr &info);

  virtual UserType getUserType() const;
  void setUserType(UserType t);

  virtual UserProtocol getUserProtocol() const;
  void setUserProtocol(UserProtocol p);

  void close();

  network::ConnectionPtr getConnection();

  uint64_t elapsedTime();

  void active();

  void deactive();

  // @brief user id string
  std::string getUserId() const;

  SessionPtr getSession();

  StreamPtr getStream() const;

protected:
  network::ConnectionPtr connection_;
  StreamPtr stream_;

  std::string domain_name_;
  std::string app_name_;
  std::string stream_name_;
  std::string param_;
  std::string user_id_;

  utils::AppInfoPtr app_info_;
  int64_t start_timestamp_{0};
  UserType type_{UserType::kUserTypeUnknown};
  UserProtocol protocol_{UserProtocol::kUserProtocolUnknown};

  std::atomic_bool destroyed_{false};
  SessionPtr session_;
};

using LiveUserPtr = std::shared_ptr<LiveUser>;

} // namespace lssvc::live

#endif
