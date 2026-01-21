#include "live/live_session.h"
#include "live/base/live_logger.h"
#include "live/live_stream.h"
#include "live/rtmp_playeruser.h"
#include "utils/lssvc_appinfo.h"
#include "utils/lssvc_string.h"
#include "utils/lssvc_time.h"
#include <memory>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::live;

namespace {
  static LiveUserPtr user_null;
}

LiveSession::LiveSession(const std::string &session_name)
    : session_name_(session_name) {
  // create a new stream
  stream_ = std::make_shared<LiveStream>(*this, session_name);
  player_live_time_.store(LSSTime::nowMs());
}

int32_t LiveSession::readyTime() const {
  return stream_->getReadyTime();
}

int64_t LiveSession::sinceStart() const {
  return stream_->sinceStart();
}

bool LiveSession::isTimeout() {
  if(stream_->isTimeout()) {
    return true;
  }
  auto idle = LSSTime::nowMs() - player_live_time_.load();

  // TODO: current session's out of time rule
  // if(publisher_ && idle > app_info_->stream_idle_time_)
  if(players_.empty() && idle > app_info_->stream_idle_time_) {
    return true;
  }
  return false;
}

LiveUserPtr LiveSession::createPublisher(const network::ConnectionPtr &conn,
                                         const std::string &session_name,
                                         const std::string &param,
                                         UserType type) {
  if(session_name != session_name_) {
    LIVE_ERROR << "create publisher failed. Invalid session name:"
               << session_name;
    return user_null;
  }
  auto list = LSSString::split(session_name, "/");
  if(list.size() != 3) {
    // domain/app/stream
    LIVE_ERROR << "create publish user failed. Invalid session name:"
               << session_name;
    return user_null;
  }
  LiveUserPtr user = std::make_shared<LiveUser>(conn, stream_, shared_from_this());
  user->setAppInfo(app_info_);
  user->setDomainName(list[0]);
  user->setAppName(list[1]);
  user->setStreamName(list[2]);
  user->setParam(param);
  user->setUserType(type);
  conn->setContext(kUserContext, user);
  return user;
}

LiveUserPtr LiveSession::createPlayerUser(const network::ConnectionPtr &conn,
                                          const std::string &session_name,
                                          const std::string &param,
                                          UserType type) {
  if (session_name != session_name_) {
    LIVE_ERROR << "create publisher failed. Invalid session name:"
               << session_name;
    return user_null;
  }
  auto list = LSSString::split(session_name, "/");
  if(list.size() != 3) {
    // domain/app/stream
    LIVE_ERROR << "create publish user failed. Invalid session name:"
               << session_name;
    return user_null;
  }
  PlayerUserPtr user;
  if(type == UserType::kUserTypePlayerRtmp) {
    user = std::make_shared<RtmpPlayerUser>(conn, stream_, shared_from_this());
  }
  user->setAppInfo(app_info_);
  user->setDomainName(list[0]);
  user->setAppName(list[1]);
  user->setStreamName(list[2]);
  user->setParam(param);
  user->setUserType(type);
  conn->setContext(kUserContext, user);
  return user;
}

void LiveSession::closeUser(const LiveUserPtr &user) {
  if (!user->destroyed_.exchange(true)) {
    // if user hasn't been destroyed
    {
      std::lock_guard<std::mutex> lk(lock_);
      if (user->getUserType() <= UserType::kUserTypePlayerWebRTC) {
        if (publisher_) {
          LIVE_DEBUG << "remove publisher, session name: " << session_name_
                     << ", user:" << user->getUserId()
                     << ", elapsed:" << user->elapsedTime()
                     << ", ready time:" << readyTime()
                     << ", stream time:" << sinceStart();
          publisher_.reset();
        }
      } else {
        LIVE_DEBUG << "remove player, session name: " << session_name_
                   << ", user:" << user->getUserId()
                   << ", elapsed:" << user->elapsedTime()
                   << ", ready time:" << readyTime()
                   << ", stream time:" << sinceStart();
        players_.erase(std::dynamic_pointer_cast<PlayerUser>(user));
        player_live_time_ = LSSTime::nowMs();
      }
    }
    user->close();
  }
}

void LiveSession::activeAllPlayers() {
  std::lock_guard<std::mutex> lk(lock_);
  for(auto const &u : players_) {
    u->active();
  }
}

void LiveSession::addPlayer(const PlayerUserPtr &user) {
  {
    std::lock_guard<std::mutex> lk(lock_);
    players_.insert(user);
  }

  LIVE_DEBUG << "add player, session name: " << session_name_ <<", user:" << user->getUserId();

  if(!publisher_) {
    // TODO
  }
  user->active();
}

void LiveSession::setPublisher(LiveUserPtr &user) {
  std::lock_guard<std::mutex> lk(lock_);
  if(publisher_ == user) {
    return;
  }

  if (publisher_ && !publisher_->destroyed_.exchange(true)) {
    publisher_->destroyed_.exchange(true);
    publisher_->close();
  }
  publisher_ = user;
}

StreamPtr LiveSession::getStream() {
  return stream_;
}

const std::string &LiveSession::getSessionName() const {
  return session_name_;
}

void LiveSession::setAppInfo(AppInfoPtr &info) {
  app_info_ = info;
}

AppInfoPtr &LiveSession::getAppInfo() {
  return app_info_;
}

bool LiveSession::isPublishing() const {
  return !!publisher_;
}

void LiveSession::clear() {
  std::lock_guard<std::mutex> lk(lock_);
  if(publisher_) {
    closeUserNoLock(publisher_);
  }
  for(auto const &p : players_) {
    closeUserNoLock(std::dynamic_pointer_cast<LiveUser>(p));
  }
  players_.clear();
}

void LiveSession::closeUserNoLock(const LiveUserPtr &user) {
  if (!user->destroyed_.exchange(true)) {
    {
      if (user->getUserType() <= UserType::kUserTypePlayerWebRTC) {
        if (publisher_) {
          LIVE_DEBUG << "remove publisher,session name:" << session_name_
                     << ",user:" << user->getUserId()
                     << ",elapsed:" << user->elapsedTime()
                     << ",ReadyTime:" << readyTime()
                     << ",stream time:" << sinceStart();

          user->close();
          publisher_.reset();
        }
      } else {
        LIVE_DEBUG << "remove publisher,session name:" << session_name_
                   << ",user:" << user->getUserId()
                   << ",elapsed:" << user->elapsedTime()
                   << ",ReadyTime:" << readyTime()
                   << ",stream time:" << sinceStart();
        players_.erase(std::dynamic_pointer_cast<PlayerUser>(user));
        user->close();
        player_live_time_ = LSSTime::nowMs();
      }
    }
  }
}
