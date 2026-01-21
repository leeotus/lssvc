#include "live/live_user.h"
#include "live/live_stream.h"
#include "utils/lssvc_time.h"

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::live;

LiveUser::LiveUser(const ConnectionPtr &ptr, const StreamPtr &stream,
                   const SessionPtr &s)
    : connection_(ptr), stream_(stream), session_(s) {
  start_timestamp_ = LSSTime::nowMs();
}

const std::string &LiveUser::getDomainName() const {
  return domain_name_;
}

void LiveUser::setDomainName(const std::string &domain) {
  domain_name_ = domain;
}

const std::string &LiveUser::getAppName() const {
  return app_name_;
}

void LiveUser::setAppName(const std::string &domain) {
  app_name_ = domain;
}

const std::string &LiveUser::getStreamName() const {
  return stream_name_;
}

void LiveUser::setStreamName(const std::string &domain) {
  stream_name_ = domain;
}

const std::string &LiveUser::getParam() const {
  return param_;
}

void LiveUser::setParam(const std::string &domain) {
  param_ = domain;
}

const AppInfoPtr &LiveUser::getAppInfo() const {
  return app_info_;
}

void LiveUser::setAppInfo(const AppInfoPtr &info) {
  app_info_ = info;
}

UserType LiveUser::getUserType() const {
  return type_;
}

void LiveUser::setUserType(UserType t) {
  type_ = t;
}

UserProtocol LiveUser::getUserProtocol() const {
  return protocol_;
}

void LiveUser::setUserProtocol(UserProtocol p) {
  protocol_ = p;
}

void LiveUser::close() {
  if(connection_) {
    connection_->forceClose();
  }
}

ConnectionPtr LiveUser::getConnection() {
  return connection_;
}

uint64_t LiveUser::elapsedTime() {
  return utils::LSSTime::nowMs() - start_timestamp_;
}

void LiveUser::active() {
  if(connection_) {
    connection_->active();
  }
}

void LiveUser::deactive() {
  if(connection_) {
    connection_->deactive();
  }
}

std::string LiveUser::getUserId() const { return user_id_; }

SessionPtr LiveUser::getSession() {
  return session_;
}

StreamPtr LiveUser::getStream() const {
  return stream_;
}
