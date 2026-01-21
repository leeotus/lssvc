#include "utils/lssvc_appinfo.h"
#include "utils/lssvc_domain_info.h"
#include "utils/lssvc_logger.h"
#include "utils/lssvc_logstream.h"

using namespace lssvc::utils;

LSSAppInfo::LSSAppInfo(LSSDomainInfo &domain) : domain_info_(domain) {}

bool LSSAppInfo::parseAppInfo(Json::Value &root) {
  Json::Value nameObj = root["name"];
  if (!nameObj.isNull()) {
    app_name_ = nameObj.asString();
  }
  Json::Value mb = root["max_buffer"];
  if (!mb.isNull()) {
    max_buffer_ = mb.asUInt();
  }
  Json::Value hls = root["hls_support"];
  if (!hls.isNull()) {
    hls_support_ = hls.asString() == "on";
  }

  Json::Value flv = root["flv_support"];
  if (!flv.isNull()) {
    flv_support_ = flv.asString() == "on";
  }

  Json::Value rtmp = root["rtmp_support"];
  if (!rtmp.isNull()) {
    rtmp_support_ = rtmp.asString() == "on";
  }

  Json::Value cl = root["content_latency"];
  if (!cl.isNull()) {
    content_latency_ = cl.asUInt() * 1000;  // seconds
  }

  Json::Value sit = root["stream_idle_time"];
  if (!sit.isNull()) {
    stream_idle_time_ = sit.asUInt();
  }

  Json::Value stt = root["stream_timeout_time"];
  if (!stt.isNull()) {
    stream_timeout_time_ = stt.asUInt();
  }

  LSSVC_LOG_INFO << "app name:" << app_name_ << " max_buffer:" << max_buffer_
                 << " content_latency:" << content_latency_
                 << " stream_idle_time:" << stream_idle_time_
                 << " stream_timeout_time:" << stream_timeout_time_
                 << " rtmp_support:" << rtmp_support_
                 << " flv_support:" << flv_support_
                 << " hls_support: " << hls_support_;

  return true;
}
