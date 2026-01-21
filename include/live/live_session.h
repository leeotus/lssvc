#ifndef __LIVE_SESSION_H__
#define __LIVE_SESSION_H__

#include "live_user.h"
#include "player_user.h"
#include "utils/lssvc_appinfo.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_set>

namespace lssvc::live {

class LiveSession : public std::enable_shared_from_this<LiveSession> {
public:
  explicit LiveSession(const std::string &session_name);

  // @brief return the timestamp when the stream of this session is ready
  int32_t readyTime() const;

  // @see LiveStream->sinceStart
  int64_t sinceStart() const;

  // @brief check whether the current session is out of time or not
  bool isTimeout();

  /**
   * @brief create the publih user for this session
   * @param conn [in] publihser connection
   * @param session_name [in] name of this session
   * @param param [in] parameter
   * @param type [in] type of publisher, @see class UserType
   */
  LiveUserPtr createPublisher(const network::ConnectionPtr &conn,
                              const std::string &session_name,
                              const std::string &param, UserType type);

  // @brief same as "createPublisher" but used for creating player_user
  LiveUserPtr createPlayerUser(const network::ConnectionPtr &conn,
                               const std::string &session_name,
                               const std::string &param, UserType type);

  // @brief close user's connection
  void closeUser(const LiveUserPtr &user);

  void activeAllPlayers();

  // @brief add a new player
  void addPlayer(const PlayerUserPtr &user);

  // @brief set the publihser of this session
  void setPublisher(LiveUserPtr &user);

  // @brief return the stream object
  StreamPtr getStream();

  // @brief return session name
  const std::string &getSessionName() const;

  // @brief set appinfo object
  void setAppInfo(utils::AppInfoPtr &info);

  utils::AppInfoPtr &getAppInfo();

  // @brief return it is publishing stream or not
  bool isPublishing() const;

  void clear();

private:
  void closeUserNoLock(const LiveUserPtr &user);

  std::string session_name_;
  std::unordered_set<PlayerUserPtr> players_;
  utils::AppInfoPtr app_info_;
  StreamPtr stream_;
  LiveUserPtr publisher_;
  std::mutex lock_;
  std::atomic<int64_t> player_live_time_;
};

}  // namespace lssvc::live

#endif
